//==================================================================================
// BSD 2-Clause License
//
// Copyright (c) 2014-2022, NJIT, Duality Technologies Inc. and other contributors
//
// All rights reserved.
//
// Author TPOC: contact@openfhe.org
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//==================================================================================

#ifndef LBCRYPTO_CRYPTO_CKKSRNS_FHE_H
#define LBCRYPTO_CRYPTO_CKKSRNS_FHE_H

#include "constants.h"
#include "encoding/plaintext-fwd.h"
#include "schemerns/rns-fhe.h"
#include "scheme/ckksrns/ckksrns-utils.h"
#include "utils/caller_info.h"

#include <memory>
#include <string>
#include <utility>
#include <map>
#include <vector>

/**
 * @namespace lbcrypto
 * The namespace of lbcrypto
 */
namespace lbcrypto {

class CKKSBootstrapPrecom {
public:
    CKKSBootstrapPrecom() {}

    CKKSBootstrapPrecom(const CKKSBootstrapPrecom& rhs) {
        m_dim1         = rhs.m_dim1;
        m_slots        = rhs.m_slots;
        m_paramsEnc    = rhs.m_paramsEnc;
        m_paramsDec    = rhs.m_paramsDec;
        m_U0Pre        = rhs.m_U0Pre;
        m_U0hatTPre    = rhs.m_U0hatTPre;
        m_U0PreFFT     = rhs.m_U0PreFFT;
        m_U0hatTPreFFT = rhs.m_U0hatTPreFFT;
    }

    CKKSBootstrapPrecom(CKKSBootstrapPrecom&& rhs) {
        m_dim1         = rhs.m_dim1;
        m_slots        = rhs.m_slots;
        m_paramsEnc    = std::move(rhs.m_paramsEnc);
        m_paramsDec    = std::move(rhs.m_paramsDec);
        m_U0Pre        = std::move(rhs.m_U0Pre);
        m_U0hatTPre    = std::move(rhs.m_U0hatTPre);
        m_U0PreFFT     = std::move(rhs.m_U0PreFFT);
        m_U0hatTPreFFT = std::move(rhs.m_U0hatTPreFFT);
    }

    virtual ~CKKSBootstrapPrecom() {}
    // the inner dimension in the baby-step giant-step strategy
    uint32_t m_dim1 = 0;

    // number of slots for which the bootstrapping is performed
    uint32_t m_slots = 0;

    // level budget for homomorphic encoding, number of layers to collapse in one level,
    // number of layers remaining to be collapsed in one level to have exactly the number
    // of levels specified in the level budget, the number of rotations in one level,
    // the baby step and giant step in the baby-step giant-step strategy, the number of
    // rotations in the remaining level, the baby step and giant step in the baby-step
    // giant-step strategy for the remaining level
    std::vector<int32_t> m_paramsEnc = std::vector<int32_t>(CKKS_BOOT_PARAMS::TOTAL_ELEMENTS, 0);

    // level budget for homomorphic decoding, number of layers to collapse in one level,
    // number of layers remaining to be collapsed in one level to have exactly the number
    // of levels specified in the level budget, the number of rotations in one level,
    // the baby step and giant step in the baby-step giant-step strategy, the number of
    // rotations in the remaining level, the baby step and giant step in the baby-step
    // giant-step strategy for the remaining level
    std::vector<int32_t> m_paramsDec = std::vector<int32_t>(CKKS_BOOT_PARAMS::TOTAL_ELEMENTS, 0);

    // Linear map U0; used in decoding
    std::vector<ConstPlaintext> m_U0Pre;

    // Conj(U0^T); used in encoding
    std::vector<ConstPlaintext> m_U0hatTPre;

    // coefficients corresponding to U0; used in decoding
    std::vector<std::vector<ConstPlaintext>> m_U0PreFFT;

    // coefficients corresponding to conj(U0^T); used in encoding
    std::vector<std::vector<ConstPlaintext>> m_U0hatTPreFFT;
};

class FHECKKSRNS : public FHERNS {
    using ParmType = typename DCRTPoly::Params;

public:
    // key tuple is dim1, levelBudgetEnc, levelBudgetDec
    std::map<uint32_t, std::shared_ptr<CKKSBootstrapPrecom>> m_bootPrecomMap;

    virtual ~FHECKKSRNS() {}

    //------------------------------------------------------------------------------
    // Bootstrap Wrapper
    //------------------------------------------------------------------------------

    void EvalBootstrapSetup(const CryptoContextImpl<DCRTPoly>& cc, std::vector<uint32_t> levelBudget,
                            std::vector<uint32_t> dim1, uint32_t slots, uint32_t correctionFactor) override;

    std::shared_ptr<std::map<usint, EvalKey<DCRTPoly>>> EvalBootstrapKeyGen(const PrivateKey<DCRTPoly> privateKey,
                                                                            uint32_t slots) override;

    Ciphertext<DCRTPoly> EvalBootstrap(ConstCiphertext<DCRTPoly> ciphertext, uint32_t numIterations,
                                       uint32_t precision) const override;

    //------------------------------------------------------------------------------
    // Find Rotation Indices
    //------------------------------------------------------------------------------

    std::vector<int32_t> FindBootstrapRotationIndices(uint32_t slots, uint32_t M);

    std::vector<int32_t> FindLinearTransformRotationIndices(uint32_t slots, uint32_t M);

    std::vector<int32_t> FindCoeffsToSlotsRotationIndices(uint32_t slots, uint32_t M);

    std::vector<int32_t> FindSlotsToCoeffsRotationIndices(uint32_t slots, uint32_t M);

    //------------------------------------------------------------------------------
    // Precomputations for CoeffsToSlots and SlotsToCoeffs
    //------------------------------------------------------------------------------

    std::vector<ConstPlaintext> EvalLinearTransformPrecompute(const CryptoContextImpl<DCRTPoly>& cc,
                                                              const std::vector<std::vector<std::complex<double>>>& A,
                                                              double scale = 1, uint32_t L = 0) const;

    std::vector<ConstPlaintext> EvalLinearTransformPrecompute(const CryptoContextImpl<DCRTPoly>& cc,
                                                              const std::vector<std::vector<std::complex<double>>>& A,
                                                              const std::vector<std::vector<std::complex<double>>>& B,
                                                              uint32_t orientation = 0, double scale = 1,
                                                              uint32_t L = 0) const;

    std::vector<std::vector<ConstPlaintext>> EvalCoeffsToSlotsPrecompute(const CryptoContextImpl<DCRTPoly>& cc,
                                                                         const std::vector<std::complex<double>>& A,
                                                                         const std::vector<uint32_t>& rotGroup,
                                                                         bool flag_i, double scale = 1,
                                                                         uint32_t L = 0) const;

    std::vector<std::vector<ConstPlaintext>> EvalSlotsToCoeffsPrecompute(const CryptoContextImpl<DCRTPoly>& cc,
                                                                         const std::vector<std::complex<double>>& A,
                                                                         const std::vector<uint32_t>& rotGroup,
                                                                         bool flag_i, double scale = 1,
                                                                         uint32_t L = 0) const;

    //------------------------------------------------------------------------------
    // EVALUATION: CoeffsToSlots and SlotsToCoeffs
    //------------------------------------------------------------------------------

    Ciphertext<DCRTPoly> EvalLinearTransform(const std::vector<ConstPlaintext>& A, ConstCiphertext<DCRTPoly> ct) const;

    Ciphertext<DCRTPoly> EvalCoeffsToSlots(const std::vector<std::vector<ConstPlaintext>>& A,
                                           ConstCiphertext<DCRTPoly> ctxt) const;

    Ciphertext<DCRTPoly> EvalSlotsToCoeffs(const std::vector<std::vector<ConstPlaintext>>& A,
                                           ConstCiphertext<DCRTPoly> ctxt) const;

    //------------------------------------------------------------------------------
    // SERIALIZATION
    //------------------------------------------------------------------------------

    template <class Archive>
    void save(Archive& ar) const {
        ar(cereal::base_class<FHERNS>(this));
    }

    template <class Archive>
    void load(Archive& ar) {
        ar(cereal::base_class<FHERNS>(this));
    }

    static uint32_t GetBootstrapDepth(uint32_t approxModDepth, const std::vector<uint32_t>& levelBudget,
                                      SecretKeyDist secretKeyDist);

    std::string SerializedObjectName() const {
        return "FHECKKSRNS";
    }

private:
    //------------------------------------------------------------------------------
    // Auxiliary Bootstrap Functions
    //------------------------------------------------------------------------------
    uint32_t GetBootstrapDepth(uint32_t approxModDepth, const std::vector<uint32_t>& levelBudget,
                               const CryptoContextImpl<DCRTPoly>& cc);

    void AdjustCiphertext(Ciphertext<DCRTPoly>& ciphertext, double correction) const;

    void ApplyDoubleAngleIterations(Ciphertext<DCRTPoly>& ciphertext) const;

    Plaintext MakeAuxPlaintext(const CryptoContextImpl<DCRTPoly>& cc, const std::shared_ptr<ParmType> params,
                               const std::vector<std::complex<double>>& value, size_t noiseScaleDeg, uint32_t level,
                               usint slots) const;

    Ciphertext<DCRTPoly> EvalMultExt(ConstCiphertext<DCRTPoly> ciphertext, ConstPlaintext plaintext) const;

    void EvalAddExtInPlace(Ciphertext<DCRTPoly>& ciphertext1, ConstCiphertext<DCRTPoly> ciphertext2) const;

    Ciphertext<DCRTPoly> EvalAddExt(ConstCiphertext<DCRTPoly> ciphertext1, ConstCiphertext<DCRTPoly> ciphertext2) const;

    EvalKey<DCRTPoly> ConjugateKeyGen(const PrivateKey<DCRTPoly> privateKey) const;

    Ciphertext<DCRTPoly> Conjugate(ConstCiphertext<DCRTPoly> ciphertext,
                                   const std::map<usint, EvalKey<DCRTPoly>>& evalKeys) const;

    /**
   * Set modulus and recalculates the vector values to fit the modulus
   *
   * @param &vec input vector
   * @param &bigValue big bound of the vector values.
   * @param &modulus modulus to be set for vector.
   */
    void FitToNativeVector(uint32_t ringDim, const std::vector<int64_t>& vec, int64_t bigBound,
                           NativeVector* nativeVec) const;

#if NATIVEINT == 128 && !defined(__EMSCRIPTEN__)
    /**
   * Set modulus and recalculates the vector values to fit the modulus
   *
   * @param &vec input vector
   * @param &bigValue big bound of the vector values.
   * @param &modulus modulus to be set for vector.
   */
    void FitToNativeVector(uint32_t ringDim, const std::vector<__int128>& vec, __int128 bigBound,
                           NativeVector* nativeVec) const;

    constexpr __int128 Max128BitValue() const {
        // 2^127-2^73-1 - max value that could be rounded to int128_t
        return ((unsigned __int128)1 << 127) - ((unsigned __int128)1 << 73) - (unsigned __int128)1;
    }

    inline bool is128BitOverflow(double d) const {
        const double EPSILON = 0.000001;

        return EPSILON < (std::abs(d) - Max128BitValue());
    }
#else  // NATIVEINT == 64
    constexpr int64_t Max64BitValue() const {
        // 2^63-2^9-1 - max value that could be rounded to int64_t
        return 9223372036854775295;
    }

    inline bool is64BitOverflow(double d) const {
        const double EPSILON = 0.000001;

        return EPSILON < (std::abs(d) - Max64BitValue());
    }
#endif

    const uint32_t K_SPARSE  = 28;   // upper bound for the number of overflows in the sparse secret case
    const uint32_t K_UNIFORM = 512;  // upper bound for the number of overflows in the uniform secret case
    static const uint32_t R =
        6;  // number of double-angle iterations in CKKS bootstrapping. Must be static because it is used in a static function.
    uint32_t m_correctionFactor = 0;  // correction factor, which we scale the message by to improve precision

    // Chebyshev series coefficients for the SPARSE case
    const std::vector<double> g_coefficientsSparse{
        0, -0.0135108039848369,   0, -0.0131996889093333,    0, -0.0125569882261219,   0, -0.0115442526766853,
        0, -0.0101103902289506,   0, -0.00820046560874961,   0, -0.00576866285480124,  0, -0.00279587498596429,
        0, 0.000688315648694393,  0, 0.00458080979047752,    0, 0.00868084082394441,   0, 0.0126725669032461,
        0, 0.0161231367224809,    0, 0.0185068535892961,     0, 0.0192657661348274,    0, 0.0179127307259012,
        0, 0.0141730394273241,    0, 0.00814448389641551,    0, 0.000435079106055255,  0, -0.00778168247898840,
        0, -0.0148620520418985,   0, -0.0189889685361429,    0, -0.0186693191727712,   0, -0.0133407437891706,
        0, -0.00389090564057687,  0, 0.00718464815465823,    0, 0.0162407979408841,    0, 0.0196382120428656,
        0, 0.0153445149479063,    0, 0.00434145249445130,    0, -0.00902722375432935,  0, -0.0184292744553373,
        0, -0.0185314839068087,   0, -0.00836260069096976,   0, 0.00703833098980111,   0, 0.0184988285845997,
        0, 0.0180654166205522,    0, 0.00500788437441869,    0, -0.0120915939476835,   0, -0.0202556523257613,
        0, -0.0120847795280861,   0, 0.00666378109657852,    0, 0.0198653249422895,    0, 0.0146140880596363,
        0, -0.00521892246765200,  0, -0.0201001486830508,    0, -0.0135644014596990,   0, 0.00843825593568634,
        0, 0.0209634381371693,    0, 0.00799557220560670,    0, -0.0154667862879796,   0, -0.0188060461698666,
        0, 0.00382389885895426,   0, 0.0213887430486565,     0, 0.00734190474839939,   0, -0.0183416505528289,
        0, -0.0150233010753339,   0, 0.0133654083836709,     0, 0.0192100548873265,    0, -0.00911978164712832,
        0, -0.0210992918227266,   0, 0.00697444852896654,    0, 0.0217610048632911,    0, -0.00750782303408115,
        0, -0.0214683224267110,   0, 0.0108811502520554,     0, 0.0194116301252710,    0, -0.0166384312124680,
        0, -0.0137472712856021,   0, 0.0227756589268448,     0, 0.00254857050817355,   0, -0.0245620739461377,
        0, 0.0133108163765930,    0, 0.0152922514820662,     0, -0.0256958257907690,   0, 0.00698277627408088,
        0, 0.0193845771351149,    0, -0.0270777073502840,    0, 0.0104725799103958,    0, 0.0149309360408655,
        0, -0.0296437418991450,   0, 0.0251457036208045,     0, -0.00626036128127151,  0, -0.0158891277144540,
        0, 0.0319777051013319,    0, -0.0383174350194706,    0, 0.0362086423459292,    0, -0.0292600279327636,
        0, 0.0209876001196013,    0, -0.0136493197252633,    0, 0.00816021896075802,   0, -0.00452840632882337,
        0, 0.00234953361489861,   0, -0.00114618395363303,   0, 0.000528114355921115,  0, -0.000230686815697107,
        0, 0.0000958302055963073, 0, -0.0000379606883316289, 0, 0.0000143724465139754, 0, -5.21172824937373e-6,
        0, 1.81332717643862e-6,   0, -6.06344859646816e-7,   0, 1.95140996547546e-7,   0, -6.05257031400827e-8,
        0, 1.81142547094519e-8,   0, -5.23689280693738e-9,   0, 1.46401288885142e-9,   0, -3.96137974049598e-10,
        0, 1.03839050405075e-10,  0, -2.63906159797951e-11,  0, 6.50787863397468e-12,  0, -1.55802645272293e-12,
        0, 3.62583753188235e-13,  0, -8.21737180817026e-14,  0, 1.80567910604366e-14,  0, -3.89583835574911e-15,
        0, 8.57038017789907e-16,  0, -1.76591223254838e-16,  0, -1.28043143258163e-16, 0, -2.99798900377873e-18,
        0, 1.00577695610641e-17,  0, 4.85480799966750e-17,   0, 1.18372210987909e-16,  0, 7.40793411901455e-17,
        0, 2.04056670902359e-16,  0, 2.81617547709796e-16,   0, 7.73674581620318e-19,  0, -7.53365623852785e-17,
        0, 6.13137105934102e-17,  0, 1.58603289232165e-16,   0, 1.30751004293834e-16,  0, 3.67495426269651e-17,
        0, 2.08892137037486e-16,  0, 2.18563069307740e-16,   0, 1.83747713134826e-16,  0, 1.93418645405080e-17,
        0, -4.37126138615480e-17, 0, 1.69821570665660e-16,   0, 1.34425958556530e-16,  0, -1.48932356961911e-16,
        0, -1.29010236485188e-16, 0, -9.51619735392991e-17,  0, 4.27455206345226e-17,  0};

    // Chebyshev series coefficients for the OPTIMIZED/uniform case
    const std::vector<double> g_coefficientsUniform{
        0.15421426400235561,    -0.0037671538417132409,  0.16032011744533031,      -0.0034539657223742453,
        0.17711481926851286,    -0.0027619720033372291,  0.19949802549604084,      -0.0015928034845171929,
        0.21756948616367638,    0.00010729951647566607,  0.21600427371240055,      0.0022171399198851363,
        0.17647500259573556,    0.0042856217194480991,   0.086174491919472254,     0.0054640252312780444,
        -0.046667988130649173,  0.0047346914623733714,   -0.17712686172280406,     0.0016205080004247200,
        -0.22703114241338604,   -0.0028145845916205865,  -0.13123089730288540,     -0.0056345646688793190,
        0.078818395388692147,   -0.0037868875028868542,  0.23226434602675575,      0.0021116338645426574,
        0.13985510526186795,    0.0059365649669377071,   -0.13918475289368595,     0.0018580676740836374,
        -0.23254376365752788,   -0.0054103844866927788,  0.056840618403875359,     -0.0035227192748552472,
        0.25667909012207590,    0.0055029673963982112,   -0.073334392714092062,    0.0027810273357488265,
        -0.24912792167850559,   -0.0069524866497120566,  0.21288810409948347,      0.0017810057298691725,
        0.088760951809475269,   0.0055957188940032095,   -0.31937177676259115,     -0.0087539416335935556,
        0.34748800245527145,    0.0075378299617709235,   -0.25116537379803394,     -0.0047285674679876204,
        0.13970502851683486,    0.0023672533925155220,   -0.063649401080083698,    -0.00098993213448982727,
        0.024597838934816905,   0.00035553235917057483,  -0.0082485030307578155,   -0.00011176184313622549,
        0.0024390574829093264,  0.000031180384864488629, -0.00064373524734389861,  -7.8036008952377965e-6,
        0.00015310015145922058, 1.7670804180220134e-6,   -0.000033066844379476900, -3.6460909134279425e-7,
        6.5276969021754105e-6,  6.8957843666189918e-8,   -1.1842811187642386e-6,   -1.2015133285307312e-8,
        1.9839339947648331e-7,  1.9372045971100854e-9,   -3.0815418032523593e-8,   -2.9013806338735810e-10,
        4.4540904298173700e-9,  4.0505136697916078e-11,  -6.0104912807134771e-10,  -5.2873323696828491e-12,
        7.5943206779351725e-11, 6.4679566322060472e-13,  -9.0081200925539902e-12,  -7.4396949275292252e-14,
        1.0057423059167244e-12, 8.1701187638005194e-15,  -1.0611736208855373e-13,  -8.9597492970451533e-16,
        1.1421575296031385e-14};
};

}  // namespace lbcrypto

#endif

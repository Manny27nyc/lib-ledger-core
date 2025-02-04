// AUTOGENERATED FILE - DO NOT MODIFY!
// This file generated by Djinni from bitcoin_like_wallet.djinni

#include "BitcoinLikeTransactionBuilder.hpp"  // my header
#include "Amount.hpp"
#include "BitcoinLikePickingStrategy.hpp"
#include "BitcoinLikeScript.hpp"
#include "BitcoinLikeTransaction.hpp"
#include "BitcoinLikeTransactionCallback.hpp"
#include "Currency.hpp"
#include "Marshal.hpp"

namespace djinni_generated {

BitcoinLikeTransactionBuilder::BitcoinLikeTransactionBuilder() : ::djinni::JniInterface<::ledger::core::api::BitcoinLikeTransactionBuilder, BitcoinLikeTransactionBuilder>("co/ledger/core/BitcoinLikeTransactionBuilder$CppProxy") {}

BitcoinLikeTransactionBuilder::~BitcoinLikeTransactionBuilder() = default;


CJNIEXPORT void JNICALL Java_co_ledger_core_BitcoinLikeTransactionBuilder_00024CppProxy_nativeDestroy(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef)
{
    try {
        DJINNI_FUNCTION_PROLOGUE1(jniEnv, nativeRef);
        delete reinterpret_cast<::djinni::CppProxyHandle<::ledger::core::api::BitcoinLikeTransactionBuilder>*>(nativeRef);
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, )
}

CJNIEXPORT jobject JNICALL Java_co_ledger_core_BitcoinLikeTransactionBuilder_00024CppProxy_native_1addInput(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef, jstring j_transactionHash, jint j_index, jint j_sequence)
{
    try {
        DJINNI_FUNCTION_PROLOGUE1(jniEnv, nativeRef);
        const auto& ref = ::djinni::objectFromHandleAddress<::ledger::core::api::BitcoinLikeTransactionBuilder>(nativeRef);
        auto r = ref->addInput(::djinni::String::toCpp(jniEnv, j_transactionHash),
                               ::djinni::I32::toCpp(jniEnv, j_index),
                               ::djinni::I32::toCpp(jniEnv, j_sequence));
        return ::djinni::release(::djinni_generated::BitcoinLikeTransactionBuilder::fromCpp(jniEnv, r));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, 0 /* value doesn't matter */)
}

CJNIEXPORT jobject JNICALL Java_co_ledger_core_BitcoinLikeTransactionBuilder_00024CppProxy_native_1addOutput(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef, jobject j_amount, jobject j_script)
{
    try {
        DJINNI_FUNCTION_PROLOGUE1(jniEnv, nativeRef);
        const auto& ref = ::djinni::objectFromHandleAddress<::ledger::core::api::BitcoinLikeTransactionBuilder>(nativeRef);
        auto r = ref->addOutput(::djinni_generated::Amount::toCpp(jniEnv, j_amount),
                                ::djinni_generated::BitcoinLikeScript::toCpp(jniEnv, j_script));
        return ::djinni::release(::djinni_generated::BitcoinLikeTransactionBuilder::fromCpp(jniEnv, r));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, 0 /* value doesn't matter */)
}

CJNIEXPORT jobject JNICALL Java_co_ledger_core_BitcoinLikeTransactionBuilder_00024CppProxy_native_1addChangePath(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef, jstring j_path)
{
    try {
        DJINNI_FUNCTION_PROLOGUE1(jniEnv, nativeRef);
        const auto& ref = ::djinni::objectFromHandleAddress<::ledger::core::api::BitcoinLikeTransactionBuilder>(nativeRef);
        auto r = ref->addChangePath(::djinni::String::toCpp(jniEnv, j_path));
        return ::djinni::release(::djinni_generated::BitcoinLikeTransactionBuilder::fromCpp(jniEnv, r));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, 0 /* value doesn't matter */)
}

CJNIEXPORT jobject JNICALL Java_co_ledger_core_BitcoinLikeTransactionBuilder_00024CppProxy_native_1excludeUtxo(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef, jstring j_transactionHash, jint j_outputIndex)
{
    try {
        DJINNI_FUNCTION_PROLOGUE1(jniEnv, nativeRef);
        const auto& ref = ::djinni::objectFromHandleAddress<::ledger::core::api::BitcoinLikeTransactionBuilder>(nativeRef);
        auto r = ref->excludeUtxo(::djinni::String::toCpp(jniEnv, j_transactionHash),
                                  ::djinni::I32::toCpp(jniEnv, j_outputIndex));
        return ::djinni::release(::djinni_generated::BitcoinLikeTransactionBuilder::fromCpp(jniEnv, r));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, 0 /* value doesn't matter */)
}

CJNIEXPORT jobject JNICALL Java_co_ledger_core_BitcoinLikeTransactionBuilder_00024CppProxy_native_1setNumberOfChangeAddresses(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef, jint j_count)
{
    try {
        DJINNI_FUNCTION_PROLOGUE1(jniEnv, nativeRef);
        const auto& ref = ::djinni::objectFromHandleAddress<::ledger::core::api::BitcoinLikeTransactionBuilder>(nativeRef);
        auto r = ref->setNumberOfChangeAddresses(::djinni::I32::toCpp(jniEnv, j_count));
        return ::djinni::release(::djinni_generated::BitcoinLikeTransactionBuilder::fromCpp(jniEnv, r));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, 0 /* value doesn't matter */)
}

CJNIEXPORT jobject JNICALL Java_co_ledger_core_BitcoinLikeTransactionBuilder_00024CppProxy_native_1setMaxAmountOnChange(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef, jobject j_amount)
{
    try {
        DJINNI_FUNCTION_PROLOGUE1(jniEnv, nativeRef);
        const auto& ref = ::djinni::objectFromHandleAddress<::ledger::core::api::BitcoinLikeTransactionBuilder>(nativeRef);
        auto r = ref->setMaxAmountOnChange(::djinni_generated::Amount::toCpp(jniEnv, j_amount));
        return ::djinni::release(::djinni_generated::BitcoinLikeTransactionBuilder::fromCpp(jniEnv, r));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, 0 /* value doesn't matter */)
}

CJNIEXPORT jobject JNICALL Java_co_ledger_core_BitcoinLikeTransactionBuilder_00024CppProxy_native_1setMinAmountOnChange(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef, jobject j_amount)
{
    try {
        DJINNI_FUNCTION_PROLOGUE1(jniEnv, nativeRef);
        const auto& ref = ::djinni::objectFromHandleAddress<::ledger::core::api::BitcoinLikeTransactionBuilder>(nativeRef);
        auto r = ref->setMinAmountOnChange(::djinni_generated::Amount::toCpp(jniEnv, j_amount));
        return ::djinni::release(::djinni_generated::BitcoinLikeTransactionBuilder::fromCpp(jniEnv, r));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, 0 /* value doesn't matter */)
}

CJNIEXPORT jobject JNICALL Java_co_ledger_core_BitcoinLikeTransactionBuilder_00024CppProxy_native_1pickInputs(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef, jobject j_strategy, jint j_sequence, jobject j_maxUtxo)
{
    try {
        DJINNI_FUNCTION_PROLOGUE1(jniEnv, nativeRef);
        const auto& ref = ::djinni::objectFromHandleAddress<::ledger::core::api::BitcoinLikeTransactionBuilder>(nativeRef);
        auto r = ref->pickInputs(::djinni_generated::BitcoinLikePickingStrategy::toCpp(jniEnv, j_strategy),
                                 ::djinni::I32::toCpp(jniEnv, j_sequence),
                                 ::djinni::Optional<std::experimental::optional, ::djinni::I32>::toCpp(jniEnv, j_maxUtxo));
        return ::djinni::release(::djinni_generated::BitcoinLikeTransactionBuilder::fromCpp(jniEnv, r));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, 0 /* value doesn't matter */)
}

CJNIEXPORT jobject JNICALL Java_co_ledger_core_BitcoinLikeTransactionBuilder_00024CppProxy_native_1sendToAddress(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef, jobject j_amount, jstring j_address)
{
    try {
        DJINNI_FUNCTION_PROLOGUE1(jniEnv, nativeRef);
        const auto& ref = ::djinni::objectFromHandleAddress<::ledger::core::api::BitcoinLikeTransactionBuilder>(nativeRef);
        auto r = ref->sendToAddress(::djinni_generated::Amount::toCpp(jniEnv, j_amount),
                                    ::djinni::String::toCpp(jniEnv, j_address));
        return ::djinni::release(::djinni_generated::BitcoinLikeTransactionBuilder::fromCpp(jniEnv, r));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, 0 /* value doesn't matter */)
}

CJNIEXPORT jobject JNICALL Java_co_ledger_core_BitcoinLikeTransactionBuilder_00024CppProxy_native_1wipeToAddress(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef, jstring j_address)
{
    try {
        DJINNI_FUNCTION_PROLOGUE1(jniEnv, nativeRef);
        const auto& ref = ::djinni::objectFromHandleAddress<::ledger::core::api::BitcoinLikeTransactionBuilder>(nativeRef);
        auto r = ref->wipeToAddress(::djinni::String::toCpp(jniEnv, j_address));
        return ::djinni::release(::djinni_generated::BitcoinLikeTransactionBuilder::fromCpp(jniEnv, r));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, 0 /* value doesn't matter */)
}

CJNIEXPORT jobject JNICALL Java_co_ledger_core_BitcoinLikeTransactionBuilder_00024CppProxy_native_1setFeesPerByte(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef, jobject j_fees)
{
    try {
        DJINNI_FUNCTION_PROLOGUE1(jniEnv, nativeRef);
        const auto& ref = ::djinni::objectFromHandleAddress<::ledger::core::api::BitcoinLikeTransactionBuilder>(nativeRef);
        auto r = ref->setFeesPerByte(::djinni_generated::Amount::toCpp(jniEnv, j_fees));
        return ::djinni::release(::djinni_generated::BitcoinLikeTransactionBuilder::fromCpp(jniEnv, r));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, 0 /* value doesn't matter */)
}

CJNIEXPORT void JNICALL Java_co_ledger_core_BitcoinLikeTransactionBuilder_00024CppProxy_native_1build(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef, jobject j_callback)
{
    try {
        DJINNI_FUNCTION_PROLOGUE1(jniEnv, nativeRef);
        const auto& ref = ::djinni::objectFromHandleAddress<::ledger::core::api::BitcoinLikeTransactionBuilder>(nativeRef);
        ref->build(::djinni_generated::BitcoinLikeTransactionCallback::toCpp(jniEnv, j_callback));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, )
}

CJNIEXPORT jobject JNICALL Java_co_ledger_core_BitcoinLikeTransactionBuilder_00024CppProxy_native_1clone(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef)
{
    try {
        DJINNI_FUNCTION_PROLOGUE1(jniEnv, nativeRef);
        const auto& ref = ::djinni::objectFromHandleAddress<::ledger::core::api::BitcoinLikeTransactionBuilder>(nativeRef);
        auto r = ref->clone();
        return ::djinni::release(::djinni_generated::BitcoinLikeTransactionBuilder::fromCpp(jniEnv, r));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, 0 /* value doesn't matter */)
}

CJNIEXPORT void JNICALL Java_co_ledger_core_BitcoinLikeTransactionBuilder_00024CppProxy_native_1reset(JNIEnv* jniEnv, jobject /*this*/, jlong nativeRef)
{
    try {
        DJINNI_FUNCTION_PROLOGUE1(jniEnv, nativeRef);
        const auto& ref = ::djinni::objectFromHandleAddress<::ledger::core::api::BitcoinLikeTransactionBuilder>(nativeRef);
        ref->reset();
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, )
}

CJNIEXPORT jobject JNICALL Java_co_ledger_core_BitcoinLikeTransactionBuilder_parseRawUnsignedTransaction(JNIEnv* jniEnv, jobject /*this*/, jobject j_currency, jbyteArray j_rawTransaction, jint j_currentBlockHeight)
{
    try {
        DJINNI_FUNCTION_PROLOGUE0(jniEnv);
        auto r = ::ledger::core::api::BitcoinLikeTransactionBuilder::parseRawUnsignedTransaction(::djinni_generated::Currency::toCpp(jniEnv, j_currency),
                                                                                                 ::djinni::Binary::toCpp(jniEnv, j_rawTransaction),
                                                                                                 ::djinni::I32::toCpp(jniEnv, j_currentBlockHeight));
        return ::djinni::release(::djinni_generated::BitcoinLikeTransaction::fromCpp(jniEnv, r));
    } JNI_TRANSLATE_EXCEPTIONS_RETURN(jniEnv, 0 /* value doesn't matter */)
}

}  // namespace djinni_generated

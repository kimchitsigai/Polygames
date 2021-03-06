/**
 * Copyright (c) Facebook, Inc. and its affiliates. All Rights Reserved.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

// Author: Dennis Soemers
// - Affiliation: Maastricht University, DKE, Digital Ludeme Project (Ludii developer)
// - Github: https://github.com/DennisSoemers/
// - Email: dennis.soemers@maastrichtuniversity.nl (or d.soemers@gmail.com)

#include "ludii_state_wrapper.h"

namespace Ludii {

Action::Action(int i, int j, int k) {
    _loc[0] = i;
    _loc[1] = j;
    _loc[2] = k;
    _hash = uint32_t(0);  // TODO
}

void LudiiStateWrapper::Initialize() {
    // TODO
}

std::unique_ptr<mcts::State> LudiiStateWrapper::clone_() const {
  return std::make_unique<LudiiStateWrapper>(*this);
}

void LudiiStateWrapper::ApplyAction(const _Action& action) {
    // TODO
}

void LudiiStateWrapper::DoGoodAction() {
 return DoRandomAction();
}

// NOTE: String descriptions of signatures of Java methods can be found by navigating
// to directory containing the .class files and using:
//
// javap -s <ClassName.class>

LudiiStateWrapper::LudiiStateWrapper(int seed, JNIEnv* jenv, LudiiGameWrapper && ludiiGameWrapper)
	: ::State(seed), jenv(jenv), ludiiGameWrapper(ludiiGameWrapper) {

	// Find our LudiiStateWrapper Java class
	ludiiStateWrapperClass = jenv->FindClass("utils/LudiiStateWrapper");

	// Find the LudiiGameWrapper Java constructor
	jmethodID ludiiStateWrapperConstructor =
			jenv->GetMethodID(ludiiStateWrapperClass, "<init>", "(Lplayer/utils/LudiiGameWrapper;)V");

	// Call our Java constructor to instantiate new object
	ludiiStateWrapperJavaObject =
			jenv->NewObject(ludiiStateWrapperClass, ludiiStateWrapperConstructor, ludiiGameWrapper.ludiiGameWrapperJavaObject);

	// Find method IDs for all the Java methods we may want to call
	legalMovesTensorsMethodID = jenv->GetMethodID(ludiiStateWrapperClass, "legalMovesTensors", "()[[I");
	numLegalMovesMethodID = jenv->GetMethodID(ludiiStateWrapperClass, "numLegalMoves", "()I");
	applyNthMoveMethodID = jenv->GetMethodID(ludiiStateWrapperClass, "applyNthMove", "(I)V");
	returnsMethodID = jenv->GetMethodID(ludiiStateWrapperClass, "returns", "(I)D");
	isTerminalMethodID = jenv->GetMethodID(ludiiStateWrapperClass, "isTerminal", "()Z");
	toTensorMethodID = jenv->GetMethodID(ludiiStateWrapperClass, "toTensor", "()[[[F");
}

LudiiStateWrapper::LudiiStateWrapper(const LudiiStateWrapper& other)
	: ::State(other), jenv(other.jenv), ludiiGameWrapper(other.ludiiGameWrapper) {

	// Find our LudiiStateWrapper Java class
	ludiiStateWrapperClass = jenv->FindClass("utils/LudiiStateWrapper");

	// Find the LudiiStateWrapper Java copy constructor
	jmethodID ludiiStateWrapperCopyConstructor =
			jenv->GetMethodID(ludiiStateWrapperClass, "<init>", "(Lplayer/utils/LudiiStateWrapper;)V");

	// Call our Java constructor to instantiate new object
	ludiiStateWrapperJavaObject =
			jenv->NewObject(ludiiStateWrapperClass, ludiiStateWrapperCopyConstructor, other.ludiiStateWrapperJavaObject);

	// We can just copy all the pointers to methods
	legalMovesTensorsMethodID = other.legalMovesTensorsMethodID;
	numLegalMovesMethodID = other.numLegalMovesMethodID;
	applyNthMoveMethodID = other.applyNthMoveMethodID;
	returnsMethodID = other.returnsMethodID;
	isTerminalMethodID = other.isTerminalMethodID;
	toTensorMethodID = other.toTensorMethodID;
}

std::vector<std::array<int, 3>> LudiiStateWrapper::LegalMovesTensors() const {
	const jobjectArray javaArrOuter = static_cast<jobjectArray>(jenv->CallObjectMethod(ludiiStateWrapperJavaObject, legalMovesTensorsMethodID));
	const jsize numLegalMoves = jenv->GetArrayLength(javaArrOuter);

	std::vector<std::array<int, 3>> matrix(numLegalMoves);
	for (jsize i = 0; i < numLegalMoves; ++i) {
		const jintArray inner = static_cast<jintArray>(jenv->GetObjectArrayElement(javaArrOuter, i));
		jint* jints = jenv->GetIntArrayElements(inner, nullptr);

        matrix[i] = {jints[0], jints[1], jints[2]};

		// Allow JVM to clean up memory now that we have our own ints
		jenv->ReleaseIntArrayElements(inner, jints, 0);
	}

	return matrix;
}

int LudiiStateWrapper::NumLegalMoves() const {
	return (int) jenv->CallIntMethod(ludiiStateWrapperJavaObject, numLegalMovesMethodID);
}

void LudiiStateWrapper::ApplyNthMove(const int n) const {
	jenv->CallVoidMethod(ludiiStateWrapperJavaObject, applyNthMoveMethodID, n);
}

double LudiiStateWrapper::Returns(const int player) const {
	return (double) jenv->CallDoubleMethod(ludiiStateWrapperJavaObject, returnsMethodID, player);
}

bool LudiiStateWrapper::IsTerminal() const {
	return (bool) jenv->CallBooleanMethod(ludiiStateWrapperJavaObject, isTerminalMethodID);
}

std::vector<std::vector<std::vector<float>>> LudiiStateWrapper::ToTensor() const {
	const jobjectArray channelsArray = static_cast<jobjectArray>(jenv->CallObjectMethod(ludiiStateWrapperJavaObject, toTensorMethodID));
	const jsize numChannels = jenv->GetArrayLength(channelsArray);

	std::vector<std::vector<std::vector<float>>> tensor(numChannels);
	for (jsize c = 0; c < numChannels; ++c) {
		const jobjectArray xArray = static_cast<jobjectArray>(jenv->GetObjectArrayElement(channelsArray, c));
		const jsize numXCoords = jenv->GetArrayLength(xArray);

		tensor[c].resize(numXCoords);
		for (jsize x = 0; x < numChannels; ++x) {
			const jfloatArray yArray = static_cast<jfloatArray>(jenv->GetObjectArrayElement(xArray, x));
			const jsize numYCoords = jenv->GetArrayLength(yArray);
			jfloat* jfloats = jenv->GetFloatArrayElements(yArray, nullptr);

			std::copy(jfloats, jfloats + numYCoords, tensor[c][x].begin());

			// Allow JVM to clean up memory now that we have our own ints
			jenv->ReleaseFloatArrayElements(yArray, jfloats, 0);
		}
	}

	return tensor;
}

}	// namespace Ludii

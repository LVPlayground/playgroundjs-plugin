// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

#ifndef PLAYGROUND_BINDINGS_GLOBAL_CALLBACKS_H_
#define PLAYGROUND_BINDINGS_GLOBAL_CALLBACKS_H_

namespace v8 {
template<typename T> class FunctionCallbackInfo;
class Value;
}

namespace bindings {

void AddEventListenerCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments);
void DispatchEventCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments);
void FrameCounterCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments);
void HasEventListenersCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments);
void HighResolutionTimeCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments);
void PawnInvokeCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments);
void ReadFileCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments);
void RemoveEventListenerCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments);
void ReportTestsFinishedCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments);
void RequireImplCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments);
void StartTraceCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments);
void StopTraceCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments);
void WaitCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments);

}  // namespace

#endif  // PLAYGROUND_BINDINGS_GLOBAL_CALLBACKS_H_

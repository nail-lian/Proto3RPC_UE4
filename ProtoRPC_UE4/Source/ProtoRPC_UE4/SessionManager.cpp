// Copyright 2015 Paddle Creek Games Inc. All Rights Reserved.
#include "ProtoRPC_UE4PrivatePCH.h"

#include "SessionManager.h"


#include "AllowWindowsPlatformTypes.h"
#include "GeneratedProtoStubs/ExampleService.pb.h"
#include "HttpRpcChannel.h"
#include "HttpRpcController.h"

SessionManager::SessionManager()
    : sessionState_(SessionState::SS_NotAuthenticated),
	  channel_(new HttpRpcChannel("http://1-dot-pcg-login.appspot.com/appengine_auth_svc")),
      controller_(new HttpRpcController()),
      authService_(new com::paddlecreekgames::AuthService::Stub(channel_)),
	  authRequest_(new com::paddlecreekgames::AuthRequest()),
	  authResponse_(new com::paddlecreekgames::AuthResponse())  {
}

SessionManager::~SessionManager() {}

void SessionManager::StartAuthentication(const FString& Username, const FString& HashedPassword, AuthCompletionDelegate* Completion) {
	// TODO(san): A lot of this is going to end up boiler-plate that should probably be generated by a protoc plugin.
	authRequest_->set_username(TCHAR_TO_UTF8(*Username));
	authRequest_->set_hash(TCHAR_TO_UTF8(*HashedPassword));
	controller_->Reset();
	authService_->Authenticate(controller_, authRequest_, authResponse_, google::protobuf::NewCallback(this, &SessionManager::AuthenticationRpcCompleted, Completion));
	sessionState_ = SessionState::SS_Authenticating;
}

void SessionManager::AuthenticationRpcCompleted(AuthCompletionDelegate* Completion) {
	if (controller_->Failed()) {
		// RPC level failure.
		sessionState_ = SessionState::SS_AuthenticationFailed;
		sessionErrorMsg_ = "Authentication RPC failure";
	} else {
		if (authResponse_->has_failedauthdata()) {
			sessionState_ = SessionState::SS_AuthenticationFailed;
			sessionErrorMsg_ = FString(authResponse_->failedauthdata().errormessage().c_str());
		}
		else if (authResponse_->has_successfullauthdata()) {
			sessionState_ = SessionState::SS_Authenticated;
			sessionCookie_ = FString(authResponse_->successfullauthdata().authtoken().c_str());
		}
		else {
			// Error. Server sent an empty msg... :(
			sessionState_ = SessionState::SS_AuthenticationFailed;
			sessionErrorMsg_ = "Empty message from server";
		}
	}
	Completion->ExecuteIfBound();
}
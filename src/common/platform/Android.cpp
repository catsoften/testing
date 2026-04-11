#include "Platform.h"
#include "Android.h"
#include "common/Defer.h"
#include "Config.h"
#include <cstring>
#include <ctime>
#include <SDL.h>
#include <jni.h>
#include <android/log.h>

namespace Platform
{
void SendIntent(ByteString action, std::optional<ByteString> data, std::optional<ByteString> extra, std::optional<ByteString> mimeType)
{
	struct CheckFailed : public std::runtime_error
	{
		using runtime_error::runtime_error;
	};
	try
	{
		auto CHECK = [](auto thing, const char *what) {
			if (!thing)
			{
				throw CheckFailed(what);
			}
			return thing;
		};
#define CHECK(a) CHECK(a, #a)
		auto *env                   = CHECK((JNIEnv *)SDL_AndroidGetJNIEnv());
		auto activityInst           = CHECK((jobject)SDL_AndroidGetActivity());
		auto activityCls            = CHECK(env->GetObjectClass(activityInst));

		auto intentCls              = CHECK(env->FindClass("android/content/Intent"));
		auto newIntent              = CHECK(env->GetMethodID(intentCls, "<init>", "()V"));
		auto intentInst             = CHECK(env->NewObject(intentCls, newIntent));

		auto intentActionFld        = CHECK(env->GetStaticFieldID(intentCls, action.c_str(), "Ljava/lang/String;"));
		auto intentAction           = CHECK(env->GetStaticObjectField(intentCls, intentActionFld));
		auto intentSetActionMth     = CHECK(env->GetMethodID(intentCls, "setAction", "(Ljava/lang/String;)Landroid/content/Intent;"));
		CHECK(env->CallObjectMethod(intentInst, intentSetActionMth, intentAction));

		if (data)
		{
			auto uriCls           = CHECK(env->FindClass("android/net/Uri"));
			auto uriParseMth      = CHECK(env->GetStaticMethodID(uriCls, "parse", "(Ljava/lang/String;)Landroid/net/Uri;"));
			auto dataStr          = CHECK((jstring)env->NewStringUTF(data->c_str()));
			auto dataUri          = CHECK(env->CallStaticObjectMethod(uriCls, uriParseMth, dataStr));
			auto intentSetDataMth = CHECK(env->GetMethodID(intentCls, "setData", "(Landroid/net/Uri;)Landroid/content/Intent;"));
			CHECK(env->CallObjectMethod(intentInst, intentSetDataMth, dataUri));
		}

		if (extra)
		{
			auto intentExtraTextFld = CHECK(env->GetStaticFieldID(intentCls, "EXTRA_TEXT", "Ljava/lang/String;"));
			auto intentExtraText    = CHECK(env->GetStaticObjectField(intentCls, intentExtraTextFld));
			auto intentPutExtraMth  = CHECK(env->GetMethodID(intentCls, "putExtra", "(Ljava/lang/String;Ljava/lang/String;)Landroid/content/Intent;"));
			auto extraStr           = CHECK((jstring)env->NewStringUTF(extra->c_str()));
			CHECK(env->CallObjectMethod(intentInst, intentPutExtraMth, intentExtraText, extraStr));

			if (mimeType)
			{
				auto mimeTypeStr      = CHECK((jstring)env->NewStringUTF(mimeType->c_str()));
				auto intentSetTypeMth = CHECK(env->GetMethodID(intentCls, "setType", "(Ljava/lang/String;)Landroid/content/Intent;"));
				CHECK(env->CallObjectMethod(intentInst, intentSetTypeMth, mimeTypeStr));
			}
		}

		auto intentCreateChooserMth = CHECK(env->GetStaticMethodID(intentCls, "createChooser", "(Landroid/content/Intent;Ljava/lang/CharSequence;)Landroid/content/Intent;"));
		auto shareIntentInst        = CHECK(env->CallStaticObjectMethod(intentCls, intentCreateChooserMth, intentInst, nullptr));

		auto startActivityMth       = CHECK(env->GetMethodID(activityCls, "startActivity", "(Landroid/content/Intent;)V"));
		env->CallVoidMethod(activityInst, startActivityMth, shareIntentInst);
	}
	catch (const CheckFailed &ex)
	{
		__android_log_print(ANDROID_LOG_ERROR, APPID, "SendIntent failed: %s", ex.what());
	}
#undef CHECK
}

void OpenURI(ByteString uri)
{
	SendIntent("ACTION_VIEW", uri, std::nullopt, std::nullopt);
}

void ShareText(ByteString text)
{
	SendIntent("ACTION_SEND", std::nullopt, text, "text/plain");
}

long unsigned int GetTime()
{
	struct timespec s;
	clock_gettime(CLOCK_MONOTONIC, &s);
	return s.tv_sec * 1000 + s.tv_nsec / 1000000;
}

ByteString ExecutableNameFirstApprox()
{
	return "/proc/self/exe";
}

bool CanUpdate()
{
	return false;
}

void SetupCrt()
{
}

std::optional<ByteString> CallActivityStringFunc(const char *funcName)
{
	ByteString result;
	struct CheckFailed : public std::runtime_error
	{
		using runtime_error::runtime_error;
	};
	try
	{
		auto CHECK = [](auto thing, const char *what) {
			if (!thing)
			{
				throw CheckFailed(what);
			}
			return thing;
		};
#define CHECK(a) CHECK(a, #a)
		auto *env              = CHECK((JNIEnv *)SDL_AndroidGetJNIEnv());
		auto activityInst      = CHECK((jobject)SDL_AndroidGetActivity());
		auto activityCls       = CHECK(env->GetObjectClass(activityInst));
		auto getClassLoaderMth = CHECK(env->GetMethodID(activityCls, "getClassLoader", "()Ljava/lang/ClassLoader;"));
		auto classLoaderInst   = CHECK(env->CallObjectMethod(activityInst, getClassLoaderMth));
		auto classLoaderCls    = CHECK(env->FindClass("java/lang/ClassLoader"));
		auto findClassMth      = CHECK(env->GetMethodID(classLoaderCls, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;"));
		auto strClassName      = CHECK(env->NewStringUTF(ByteString::Build(APPID, ".PowderActivity").c_str()));
		Defer deleteStrClassName([env, strClassName]() { env->DeleteLocalRef(strClassName); });
		auto mPowderActivity   = CHECK((jclass)(env->CallObjectMethod(classLoaderInst, findClassMth, strClassName)));
		auto funcMth           = CHECK(env->GetMethodID(mPowderActivity, funcName, "()Ljava/lang/String;"));
		auto resultRef         = CHECK((jstring)env->CallObjectMethod(activityInst, funcMth));
		Defer deleteStr([env, resultRef]() { env->DeleteLocalRef(resultRef); });
		auto *resultBytes      = CHECK(env->GetStringUTFChars(resultRef, nullptr));
		Defer deleteUtf([env, resultRef, resultBytes]() { env->ReleaseStringUTFChars(resultRef, resultBytes); });
		result = resultBytes;
	}
	catch (const CheckFailed &ex)
	{
		__android_log_print(ANDROID_LOG_ERROR, APPID, "CallActivityStringFunc/%s failed: %s", funcName, ex.what());
		return std::nullopt;
	}
#undef CHECK
	return result;
}

ByteString DefaultDdir()
{
	auto result = CallActivityStringFunc("getDefaultDdir");
	if (result)
	{
		__android_log_print(ANDROID_LOG_ERROR, APPID, "DefaultDdir succeeded, data dir is %s", result->c_str());
		return *result;
	}
	return "";
}

int InvokeMain(int argc, char *argv[])
{
	struct CheckFailed : public std::runtime_error
	{
		using runtime_error::runtime_error;
	};
	try
	{
		auto CHECK = [](auto thing, const char *what) {
			if (!thing)
			{
				throw CheckFailed(what);
			}
			return thing;
		};
#define CHECK(a) CHECK(a, #a)
		auto *env                = CHECK((JNIEnv *)SDL_AndroidGetJNIEnv());
		auto activityInst        = CHECK((jobject)SDL_AndroidGetActivity());
		auto activityCls         = CHECK(env->GetObjectClass(activityInst));

		auto getIntentMth        = CHECK(env->GetMethodID(activityCls, "getIntent", "()Landroid/content/Intent;"));
		auto intentInst          = CHECK(env->CallObjectMethod(activityInst, getIntentMth));
		auto intentCls           = CHECK(env->GetObjectClass(intentInst));

		auto getActionMth        = CHECK(env->GetMethodID(intentCls, "getAction", "()Ljava/lang/String;"));
		auto actionStr           = CHECK((jstring)env->CallObjectMethod(intentInst, getActionMth));

		auto intentActionViewFld = CHECK(env->GetStaticFieldID(intentCls, "ACTION_VIEW", "Ljava/lang/String;"));
		auto intentActionViewStr = CHECK((jstring)env->GetStaticObjectField(intentCls, intentActionViewFld));

		auto stringCls           = CHECK(env->GetObjectClass(actionStr));
		auto stringEqualsMth     = CHECK(env->GetMethodID(stringCls, "equals", "(Ljava/lang/Object;)Z"));
		auto actionViewBool      = (jboolean)env->CallBooleanMethod(actionStr, stringEqualsMth, intentActionViewStr);

		if (actionViewBool == JNI_TRUE)
		{
			auto getDataMth     = CHECK(env->GetMethodID(intentCls, "getData", "()Landroid/net/Uri;"));
			auto uriInst        = CHECK(env->CallObjectMethod(intentInst, getDataMth));
			auto uriCls         = CHECK(env->GetObjectClass(uriInst));

			auto uriToStringMth = CHECK(env->GetMethodID(uriCls, "toString", "()Ljava/lang/String;"));
			auto resultRef      = CHECK((jstring)env->CallObjectMethod(uriInst, uriToStringMth));
			auto *resultBytes   = CHECK(env->GetStringUTFChars(resultRef, nullptr));
			Defer deleteUtf([env, resultRef, resultBytes]() { env->ReleaseStringUTFChars(resultRef, resultBytes); });
			ByteString uri = resultBytes;

			if (uri.Contains('='))
			{
				uri = "ptsave:" + uri.PartitionBy('=')[1];
			}
			else if (uri.Contains('~'))
			{
				uri = "ptsave:" + uri.PartitionBy('~')[1];
			}

			char *newArgv[2];
			newArgv[0] = argv[0];
			newArgv[1] = new char[uri.length() + 1];
			std::strcpy(newArgv[1], uri.c_str());

			return Main(2, newArgv);
		}
	}
	catch (const CheckFailed &ex)
	{
		__android_log_print(ANDROID_LOG_ERROR, APPID, "InvokeMain (intent check) failed: %s", ex.what());
	}
#undef CHECK
	return Main(argc, argv);
}

void MarkPresentable()
{
}
}

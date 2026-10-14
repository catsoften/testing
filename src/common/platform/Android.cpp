#include "Platform.h"
#include "Android.h"
#include "common/Defer.h"
#include "Config.h"
#include <cstring>
#include <ctime>
#include <SDL.h>
#include <jni.h>
#include <android/log.h>

struct CheckFailed : public std::runtime_error
{
	using runtime_error::runtime_error;
};

auto CHECK(auto thing, const char *what)
{
	if (!thing)
	{
		throw CheckFailed(what);
	}
	return thing;
};
#define CHECK(a) CHECK(a, #a)

namespace Platform
{
namespace JNI
{
JNIEnv *env;
jclass activityCls, stringCls, uriCls, intentCls;
jobject activityInst;

bool allow = false;

void Setup()
{
	env          = CHECK((JNIEnv *)SDL_AndroidGetJNIEnv());

	activityInst = CHECK((jobject)SDL_AndroidGetActivity());
	activityCls  = CHECK(env->GetObjectClass(activityInst));

	stringCls    = CHECK(env->FindClass("java/lang/String"));
	uriCls       = CHECK(env->FindClass("android/net/Uri"));
	intentCls    = CHECK(env->FindClass("android/content/Intent"));

	allow = true;
}

jobject GetStringField(jclass cls, ByteString name)
{
	auto field = CHECK(env->GetStaticFieldID(cls, name.c_str(), "Ljava/lang/String;"));
	return CHECK(env->GetStaticObjectField(cls, field));
}

jobject GetApplicationContext()
{
	auto getApplicationContextMth = CHECK(env->GetMethodID(activityCls, "getApplicationContext", "()Landroid/content/Context;"));
	return CHECK(env->CallObjectMethod(activityInst, getApplicationContextMth));
}

ByteString PathToUri(ByteString path)
{
	// Context contextInst = getApplicationContext();
	auto contextInst       = GetApplicationContext();

	// String packageNameStr = contextInst.getPackageName();
	auto contextCls        = CHECK(env->FindClass("android/content/Context"));
	auto getPackageNameMth = CHECK(env->GetMethodID(contextCls, "getPackageName", "()Ljava/lang/String;"));
	auto packageNameStr    = CHECK((jstring)env->CallObjectMethod(contextInst, getPackageNameMth));

	// String authorityStr = packageNameStr + ".provider";
	ByteString authority   = ByteString::Build(CHECK(env->GetStringUTFChars(packageNameStr, nullptr)), ".provider");
	auto authorityStr      = CHECK((jstring)env->NewStringUTF(authority.c_str()));

	// String pathStr = <path>;
	auto pathStr           = CHECK((jstring)env->NewStringUTF(path.c_str()));

	// File fileInst = new File(pathStr);
	auto fileCls           = CHECK(env->FindClass("java/io/File"));
	auto newFile           = CHECK(env->GetMethodID(fileCls, "<init>", "(Ljava/lang/String;)V"));
	auto fileInst          = CHECK(env->NewObject(fileCls, newFile, pathStr));

	// Uri uriInst = FileProvider.getUriForFile(contextInst, authorityStr, fileInst);
	auto fileProviderCls   = CHECK(env->FindClass("androidx/core/content/FileProvider"));
	auto getUriForFileMth  = CHECK(env->GetStaticMethodID(fileProviderCls, "getUriForFile", "(Landroid/content/Context;Ljava/lang/String;Ljava/io/File;)Landroid/net/Uri;"));
	auto uriInst           = CHECK(env->CallStaticObjectMethod(fileProviderCls, getUriForFileMth, contextInst, authorityStr, fileInst));

	// String uriStr = uriInst.toString();
	auto toStringMth       = CHECK(env->GetMethodID(uriCls, "toString", "()Ljava/lang/String;"));
	auto uriStr            = CHECK((jstring)env->CallObjectMethod(uriInst, toStringMth));
	return CHECK(env->GetStringUTFChars(uriStr, nullptr));
}

class Intent
{
	// Intent intentInst;
	jobject intentInst;

public:
	Intent()
	{
		// intentInst = getIntent();
		auto getIntentMth = CHECK(env->GetMethodID(activityCls, "getIntent", "()Landroid/content/Intent;"));
		intentInst        = CHECK(env->CallObjectMethod(activityInst, getIntentMth));
	}

	Intent(ByteString action)
	{
		// intentInst = new Intent();
		auto newIntent    = CHECK(env->GetMethodID(intentCls, "<init>", "()V"));
		intentInst        = CHECK(env->NewObject(intentCls, newIntent));

		// intentInst.setAction(Intent.<action>);
		auto setActionMth = CHECK(env->GetMethodID(intentCls, "setAction", "(Ljava/lang/String;)Landroid/content/Intent;"));
		CHECK(env->CallObjectMethod(intentInst, setActionMth, GetStringField(intentCls, action)));
	}

	bool IsAction(ByteString action)
	{
		// String actionStr = intentInst.getAction();
		auto getActionMth    = CHECK(env->GetMethodID(intentCls, "getAction", "()Ljava/lang/String;"));
		auto actionStr       = CHECK((jstring)env->CallObjectMethod(intentInst, getActionMth));

		// return actionStr.equals(Intent.<action>);
		auto stringEqualsMth = CHECK(env->GetMethodID(stringCls, "equals", "(Ljava/lang/Object;)Z"));
		return (jboolean)env->CallBooleanMethod(actionStr, stringEqualsMth, GetStringField(intentCls, action));
	}

	void SetType(ByteString type)
	{
		// String typeStr = <type>;
		auto typeStr    = CHECK((jstring)env->NewStringUTF(type.c_str()));

		// intentInst.setType(typeStr);
		auto setTypeMth = CHECK(env->GetMethodID(intentCls, "setType", "(Ljava/lang/String;)Landroid/content/Intent;"));
		CHECK(env->CallObjectMethod(intentInst, setTypeMth, typeStr));
	}

	void SetData(ByteString data)
	{
		// String dataStr = <data>;
		auto dataStr     = CHECK((jstring)env->NewStringUTF(data.c_str()));

		// Uri dataUri = Uri.parse(dataStr);
		auto parseMth    = CHECK(env->GetStaticMethodID(uriCls, "parse", "(Ljava/lang/String;)Landroid/net/Uri;"));
		auto dataUri     = CHECK(env->CallStaticObjectMethod(uriCls, parseMth, dataStr));

		// intentInst.setData(dataUri);
		auto setDataMth  = CHECK(env->GetMethodID(intentCls, "setData", "(Landroid/net/Uri;)Landroid/content/Intent;"));
		CHECK(env->CallObjectMethod(intentInst, setDataMth, dataUri));
	}

	ByteString GetData()
	{
		// Uri dataUri = intentInst.getData();
		auto getDataMth  = CHECK(env->GetMethodID(intentCls, "getData", "()Landroid/net/Uri;"));
		auto dataUri     = CHECK(env->CallObjectMethod(intentInst, getDataMth));

		// return dataUri.toString();
		auto toStringMth = CHECK(env->GetMethodID(uriCls, "toString", "()Ljava/lang/String;"));
		auto resultRef   = CHECK((jstring)env->CallObjectMethod(dataUri, toStringMth));
		return CHECK(env->GetStringUTFChars(resultRef, nullptr));
	}

	void PutExtraString(ByteString name, ByteString extra)
	{
		// String extraStr = <extra>;
		auto extraStr    = CHECK((jstring)env->NewStringUTF(extra.c_str()));

		// intentInst.putExtra(Intent.<name>, extraStr);
		auto putExtraMth = CHECK(env->GetMethodID(intentCls, "putExtra", "(Ljava/lang/String;Ljava/lang/String;)Landroid/content/Intent;"));
		CHECK(env->CallObjectMethod(intentInst, putExtraMth, GetStringField(intentCls, name), extraStr));
	}

	void PutExtraURI(ByteString name, ByteString extra)
	{
		// String extraStr = <extra>;
		auto extraStr    = CHECK((jstring)env->NewStringUTF(extra.c_str()));

		// Uri extraUri = Uri.parse(extraStr);
		auto parseMth    = CHECK(env->GetStaticMethodID(uriCls, "parse", "(Ljava/lang/String;)Landroid/net/Uri;"));
		auto extraUri    = CHECK(env->CallStaticObjectMethod(uriCls, parseMth, extraStr));

		// intentInst.putExtra(Intent.<name>, extraUri);
		auto putExtraMth = CHECK(env->GetMethodID(intentCls, "putExtra", "(Ljava/lang/String;Landroid/os/Parcelable;)Landroid/content/Intent;"));
		CHECK(env->CallObjectMethod(intentInst, putExtraMth, GetStringField(intentCls, name), extraUri));
	}

	void Start()
	{
		// Intent shareIntent = Intent.createChooser(intentInst, null);
		auto createChooserMth = CHECK(env->GetStaticMethodID(intentCls, "createChooser", "(Landroid/content/Intent;Ljava/lang/CharSequence;)Landroid/content/Intent;"));
		auto shareIntent      = CHECK(env->CallStaticObjectMethod(intentCls, createChooserMth, intentInst, nullptr));

		// startActivity(shareIntent);
		auto startActivityMth = CHECK(env->GetMethodID(activityCls, "startActivity", "(Landroid/content/Intent;)V"));
		env->CallVoidMethod(activityInst, startActivityMth, shareIntent);
	}
};
};

void OpenURI(ByteString uri)
{
	if (!JNI::allow)
	{
		return;
	}

	try
	{
		JNI::Intent intent("ACTION_VIEW");
		intent.SetData(uri);
		intent.Start();
	}
	catch (const CheckFailed &ex)
	{
		__android_log_print(ANDROID_LOG_ERROR, APPID, "OpenURI(%s) failed: %s", uri.c_str(), ex.what());
	}
}

void ShareText(ByteString text)
{
	if (!JNI::allow)
	{
		return;
	}

	try
	{
		JNI::Intent intent("ACTION_SEND");
		intent.SetType("text/plain");
		intent.PutExtraString("EXTRA_TEXT", text);
		intent.Start();
	}
	catch (const CheckFailed &ex)
	{
		__android_log_print(ANDROID_LOG_ERROR, APPID, "ShareText(%s) failed: %s", text.c_str(), ex.what());
	}
}

void ShareFile(ByteString path)
{
	if (!JNI::allow)
	{
		return;
	}

	try
	{
		JNI::Intent intent("ACTION_SEND");
		intent.SetType("application/vnd.powdertoy.save");
		intent.PutExtraURI("EXTRA_STREAM", JNI::PathToUri(path));
		intent.Start();
	}
	catch (const CheckFailed &ex)
	{
		__android_log_print(ANDROID_LOG_ERROR, APPID, "ShareFile(%s) failed: %s", path.c_str(), ex.what());
	}
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
	try
	{
		JNI::Setup(); // Temporary, InvokeMain isn't called
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
	try
	{
		JNI::Setup();

		if (JNI::allow)
		{
			JNI::Intent intent;
			if (intent.IsAction("ACTION_VIEW"))
			{
				ByteString uri = intent.GetData();;

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
	}
	catch (const CheckFailed &ex)
	{
		__android_log_print(ANDROID_LOG_ERROR, APPID, "InvokeMain (intent check) failed: %s", ex.what());
	}
	return Main(argc, argv);
}

void MarkPresentable()
{
}

#undef CHECK
}

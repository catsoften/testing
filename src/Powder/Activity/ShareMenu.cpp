#include "ShareMenu.hpp"
#include "common/platform/Platform.h"
#include "Gui/Icons.hpp"
#include "Gui/ViewUtil.hpp"
#include "PowderToySDL.h"
#include "Preview.hpp"

namespace Powder::Activity
{
	namespace
	{
		constexpr Gui::View::Size dialogWidth = 150;
	}

	ShareMenu::ShareMenu(Preview &newPreview) :
		View(newPreview.GetHost()),
		preview(newPreview),
		id(newPreview.GetId())
	{

	}

	void ShareMenu::Gui()
	{
		auto dialog = ScopedDialog("shareMenu", "DEFAULT_LANG_SHAREMENU_TITLE"_St, dialogWidth);
		{
			SetPrimaryAxis(Axis::vertical);
			BeginButton("share", BuildString(Gui::iconShare, " ", "DEFAULT_LANG_SHAREMENU_SHARE"_St), ButtonFlags::none);
			SetSize(30);
			if (EndButton())
			{
				Platform::ShareText(ByteString::Build("https://tpt.io/~", id));
				Exit();
			}
			BeginButton("copy", BuildString(Gui::iconCopy, " ", "DEFAULT_LANG_SHAREMENU_COPY"_St), ButtonFlags::none);
			SetSize(30);
			if (IsHoldOrRightClicked())
			{
				ClipboardPush(String::Build("id:", id).ToUtf8());
				Exit();
			}
			if (EndButton())
			{
				ClipboardPush(String::Build(id).ToUtf8());
				Exit();
			}
			BeginButton("open", BuildString(Gui::iconOpen, " ", "DEFAULT_LANG_SHAREMENU_OPEN"_St), ButtonFlags::none);
			SetSize(30);
			if (EndButton())
			{
				Platform::OpenURI(ByteString::Build("https://powdertoy.co.uk/Browse/View.html?ID=", id));
				Exit();
			}
		}
	}

	ShareMenu::DispositionFlags ShareMenu::GetDisposition() const
	{
		return DispositionFlags::cancelMissing;
	}

	void ShareMenu::Ok()
	{
		Exit();
	}
};

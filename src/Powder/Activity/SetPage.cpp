#include "SetPage.hpp"
#include "Browser.hpp"
#include "Gui/Host.hpp"
#include "Gui/Icons.hpp"

namespace Powder::Activity
{
	namespace
	{
		constexpr Gui::View::Size dialogWidth = 250;
	}

	SetPage::SetPage(Browser &newBrowser) :
		View(newBrowser.GetHost()),
		browser(newBrowser),
		newPage(newBrowser.GetPage())
	{

	}

	void SetPage::Gui()
	{
		auto &cm = GetHost().GetCommonMetrics();
		auto &pc = browser.GetPaginationContext();

		auto dialog = ScopedDialog("setPage", "DEFAULT_LANG_SETPAGE_SELECTPAGE"_St, dialogWidth);
		{
			SetPrimaryAxis(Axis::horizontal);
			SetSize(cm.heightToFitSize);
			BeginButton("prev", Gui::iconLeft, ButtonFlags::none);
			SetEnabled(newPage > 0);
			SetSize(cm.smallButton);
			if (IsHoldOrRightClicked())
			{
				newPage = 0;
				pc.ForceRead();
			}
			if (EndButton())
			{
				newPage--;
				pc.ForceRead();
			}
			struct Rw
			{
				SetPage &view;
				int32_t Read() { return view.newPage; }
				void Write(int32_t value) { view.newPage = value; }
			};
			pc.Gui("page", Rw{ *this });
			BeginButton("next", Gui::iconRight, ButtonFlags::none);
			SetEnabled(newPage < pc.GetPageCount());
			SetSize(cm.smallButton);
			if (IsHoldOrRightClicked())
			{
				newPage = pc.GetPageCount() - 1;
				pc.ForceRead();
			}
			if (EndButton())
			{
				newPage++;
				pc.ForceRead();
			}
		}
	}

	SetPage::DispositionFlags SetPage::GetDisposition() const
	{
		return browser.GetPaginationContext().HasNumber() ? DispositionFlags::none : DispositionFlags::okDisabled;
	}

	void SetPage::Ok()
	{
		browser.SetPage(newPage);
		Exit();
	}
};

#pragma once
#include "Gui/View.hpp"

namespace Powder::Activity
{
	class Browser;

	class SetPage : public Gui::View
	{
		Browser &browser;

		int32_t newPage;

		void Gui() final override;
		DispositionFlags GetDisposition() const final override;
		void Ok() final override;

	public:
		SetPage(Browser &newBrowser);
	};
};

#pragma once
#include "Gui/View.hpp"

namespace Powder::Activity
{
	class Preview;

	class ShareMenu : public Gui::View
	{
		Preview &preview;

		int32_t id;

		void Gui() final override;
		DispositionFlags GetDisposition() const final override;
		void Ok() final override;

	public:
		ShareMenu(Preview &newPreview);
	};
};

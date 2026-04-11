#include "View.hpp"
#include "Host.hpp"
#include "SdlAssert.hpp"

namespace Powder::Gui
{
	namespace
	{
		constexpr View::Size boxSize      = 13;
		constexpr View::Size boxSizeLarge = 24;
		constexpr View::Size boxSpacing   =  4;

		using SanePoint = Gui::View::ExtendedSize2<int32_t>;

		template<int size>
		constexpr auto MakeBitmap(auto &&thing)
		{
			auto bitmap = std::to_array(thing);
			static_assert(int32_t(bitmap.size() - 1) == size * size);
			return bitmap;
		}

		constexpr auto roundEdgeBitmap = MakeBitmap<boxSize>(
			"    #####    "
			"  ##     ##  "
			" #         # "
			" #         # "
			"#           #"
			"#           #"
			"#           #"
			"#           #"
			"#           #"
			" #         # "
			" #         # "
			"  ##     ##  "
			"    #####    "
		);

		constexpr auto roundEdgeLargeBitmap = MakeBitmap<boxSizeLarge>(
			"                        "
			"                        "
			"        ########        "
			"      ###      ###      "
			"     ##          ##     "
			"    ##            ##    "
			"   ##              ##   "
			"   #                #   "
			"  ##                ##  "
			"  #                  #  "
			"  #                  #  "
			"  #                  #  "
			"  #                  #  "
			"  #                  #  "
			"  #                  #  "
			"  ##                ##  "
			"   #                #   "
			"   ##              ##   "
			"    ##            ##    "
			"     ##          ##     "
			"      ###      ###      "
			"        ########        "
			"                        "
			"                        "
		);

		constexpr auto roundBulletBitmap = MakeBitmap<boxSize>(
			"             "
			"             "
			"             "
			"    #####    "
			"   #######   "
			"   #######   "
			"   #######   "
			"   #######   "
			"   #######   "
			"    #####    "
			"             "
			"             "
			"             "
		);

		constexpr auto roundBulletLargeBitmap = MakeBitmap<boxSizeLarge>(
			"                        "
			"                        "
			"                        "
			"                        "
			"                        "
			"                        "
			"                        "
			"         ######         "
			"        ########        "
			"       ##########       "
			"       ##########       "
			"       ##########       "
			"       ##########       "
			"       ##########       "
			"       ##########       "
			"        ########        "
			"         ######         "
			"                        "
			"                        "
			"                        "
			"                        "
			"                        "
			"                        "
			"                        "
		);

		template<auto Bitmap>
		constexpr int32_t GetPointListSize()
		{
			int32_t size = 0;
			for (int32_t i = 0; i < int32_t(Bitmap.size() - 1); ++i)
			{
				size += Bitmap[i] == '#';
			}
			return size;
		}

		template<auto Bitmap, int size>
		constexpr auto MakePointList()
		{
			std::array<SanePoint, GetPointListSize<Bitmap>()> pointList;
			int32_t cursor = 0;
			for (int32_t i = 0; i < int32_t(Bitmap.size() - 1); ++i)
			{
				if (Bitmap[i] == '#')
				{
					pointList[cursor] = { i / size, i % size };
					cursor += 1;
				}
			}
			return pointList;
		}
	}

	void View::BeginCheckbox(ComponentKey key, StringView text, bool &checked, CheckboxFlags checkboxFlags, Rgba8 color)
	{
		auto &g = GetHost();
		auto edgeColor = 0xFFFFFFFF_argb;
		auto size = g.IfTouchUi(boxSizeLarge, boxSize);
		BeginComponent(key);
		SetPrimaryAxis(Axis::vertical);
		auto enabled = GetCurrentComponent()->prevContent.enabled;
		if (enabled)
		{
			SetHandleButtons(true);
		}
		if (!enabled)
		{
			edgeColor.Alpha /= 2;
			color.Alpha /= 2;
		}
		auto &parentComponent = *GetParentComponent();
		if (parentComponent.prevLayout.primaryAxis == Axis::horizontal)
		{
			SetMinSizeSecondary(size);
		}
		else
		{
			SetMinSize(size);
		}
		if (!text.Empty())
		{
			auto textFlags = TextFlags::none;
			if (CheckboxFlagBase(checkboxFlags) & CheckboxFlagBase(CheckboxFlags::multiline))
			{
				textFlags = textFlags | TextFlags::autoHeight | TextFlags::multiline;
			}
			else if (CheckboxFlagBase(checkboxFlags) & CheckboxFlagBase(CheckboxFlags::autoWidth))
			{
				textFlags = textFlags | TextFlags::autoWidth;
			}
			SetTextAlignment(Alignment::left, Alignment::center);
			SetTextPadding(0, 1);
			BeginText(key, text, textFlags, color);
			auto &component = *GetParentComponent();
			SetTextPadding(
				component.prevContent.textPaddingBefore.X,
				component.prevContent.textPaddingAfter.X,
				component.prevContent.textPaddingBefore.Y,
				component.prevContent.textPaddingAfter.Y
			);
			EndText();
		}
		auto r = GetRect();
		SetPadding(0, 0, size + (text.Empty() ? 0 : boxSpacing), 0);
		Rect rr{ r.TopLeft(), { size, size } };
		auto round = bool(CheckboxFlagBase(checkboxFlags) & CheckboxFlagBase(CheckboxFlags::round));
		if (round)
		{
			if (g.GetTouchUi())
			{
				constexpr auto pointList = MakePointList<roundEdgeLargeBitmap, boxSizeLarge>();
				for (auto &p : pointList)
				{
					g.DrawPoint({ rr.pos.X + p.X, rr.pos.Y + p.Y }, edgeColor);
				}
			}
			else
			{
				constexpr auto pointList = MakePointList<roundEdgeBitmap, boxSize>();
				for (auto &p : pointList)
				{
					g.DrawPoint({ rr.pos.X + p.X, rr.pos.Y + p.Y }, edgeColor);
				}
			}
		}
		else
		{
			g.FillRect(rr.Inset(g.IfTouchUi(2, 0)), 0xFF000000_argb);
			g.DrawRect(rr.Inset(g.IfTouchUi(2, 0)), edgeColor);
		}
		if (IsClicked(SDL_BUTTON_LEFT))
		{
			checked = !checked;
		}
		if (checked)
		{
			if (round)
			{
				if (g.GetTouchUi())
				{
					static constexpr auto pointList = MakePointList<roundBulletLargeBitmap, boxSizeLarge>();
					for (auto &p : pointList)
					{
						g.DrawPoint({ rr.pos.X + p.X, rr.pos.Y + p.Y }, edgeColor);
					}
				}
				else
				{
					static constexpr auto pointList = MakePointList<roundBulletBitmap, boxSize>();
					for (auto &p : pointList)
					{
						g.DrawPoint({ rr.pos.X + p.X, rr.pos.Y + p.Y }, edgeColor);
					}
				}
			}
			else
			{
				g.FillRect(rr.Inset(g.IfTouchUi(6, 3)), edgeColor);
			}
		}
	}

	bool View::EndCheckbox()
	{
		bool clicked = IsClicked(SDL_BUTTON_LEFT);
		EndComponent();
		return clicked;
	}

	bool View::Checkbox(ComponentKey key, StringView text, bool &checked, SetSizeSize size, CheckboxFlags checkboxFlags)
	{
		BeginCheckbox(key, text, checked, checkboxFlags);
		SetSize(size);
		if (std::holds_alternative<SpanAll>(size))
		{
			SetMinSize(GetHost().IfTouchUi(boxSizeLarge, boxSize));
		}
		return EndCheckbox();
	}
}

#include "ElementSearch.hpp"
#include "Common/Log.hpp"
#include "Game.hpp"
#include "Gui/Host.hpp"
#include "Gui/Icons.hpp"
#include "Gui/SdlAssert.hpp"
#include "Gui/StaticTexture.hpp"
#include "Gui/ViewUtil.hpp"
#include "simulation/SimulationData.h"
#include "gui/game/tool/Tool.h"

namespace Powder::Activity
{
	namespace
	{
		constexpr Gui::View::Size toolButtonPadding =  2;
		constexpr Gui::View::Size toolButtonSpacing =  1;
	}

	ElementSearch::ElementSearch(Game &newGame) :
		View(newGame.GetHost()),
		game(newGame)
	{
		toolAtlasTexture = game.MakeToolAtlasTexture(*this);
		InitActions();
		Update();
	}

	void ElementSearch::InitActions()
	{
		auto rootContext = std::make_shared<ActionContext>();
		rootContext->name = Game::actionContextNamePrefix + ByteString("ROOT");
		auto initDraw = [&](int32_t index, int32_t button) {
			auto action = std::make_shared<Action>();
			action->name = Game::actionNamePrefix + ByteString::Build("DRAW", index);
			action->begin = [this, index]() {
				if (lastHoveredTool)
				{
					if (toggleFavorite)
					{
						game.ToggleFavorite(lastHoveredTool);
					}
					else if (!GetHost().GetTouchUi())
					{
						game.SelectTool(index, lastHoveredTool);
						Exit();
					}
					return InputDisposition::exclusive;
				}
				return InputDisposition::unhandled;
			};
			action->end = [this, index]() {
				if (GetHost().GetTouchUi() && lastHoveredTool && !toggleFavorite)
				{
					game.SelectTool(index, lastHoveredTool);
					Exit();
				}
			};
			rootContext->inputToActions.push_back({ MouseButtonInput{ button }, {}, action });
		};
		// TODO-REDO_UI: somehow sync with Game
		initDraw(0, SDL_BUTTON_LEFT);
		initDraw(1, SDL_BUTTON_RIGHT);
		initDraw(2, SDL_BUTTON_MIDDLE);
		SetCurrentActionContext(rootContext);
	}

	void ElementSearch::Update()
	{
		auto toLower = [](const std::string &str) {
			return std::string(ByteString(str).ToLower()); // TODO-REDO_UI
		};
		auto queryLower = toLower(query);

		enum class MatchingProperty
		{
			name,
			description,
			menuDescription,
		};
		enum class MatchKind
		{
			equals,
			beginsWith,
			contains,
		};
		struct Match
		{
			MatchingProperty matchingProperty;
			MatchKind matchKind;
			int32_t toolIndex;

			auto operator <=>(const Match &) const = default;
		};

		auto &tools = game.GetTools();
		std::vector<std::optional<Match>> matches(tools.size());
		auto found = [&](Match newMatch) {
			auto &match = matches[newMatch.toolIndex];
			if (!match)
			{
				match = newMatch;
			}
			*match = std::min(*match, newMatch);
		};

		auto matchProperty = [&](std::string infoLower, MatchingProperty matchingProperty, int32_t toolIndex) {
			if (infoLower == queryLower)
			{
				found({ matchingProperty, MatchKind::equals, toolIndex });
			}
			auto pos = infoLower.find(queryLower);
			if (pos == 0)
			{
				found({ matchingProperty, MatchKind::beginsWith, toolIndex });
			}
			if (pos != infoLower.npos)
			{
				found({ matchingProperty, MatchKind::contains, toolIndex });
			}
		};

		auto &sd = SimulationData::CRef();
		for (int32_t toolIndex = 0; toolIndex < int32_t(tools.size()); ++toolIndex)
		{
			auto &info = tools[toolIndex];
			if (!info)
			{
				continue;
			}
			matchProperty(toLower(info->tool->Name.ToUtf8()), MatchingProperty::name, toolIndex);
			matchProperty(toLower(info->tool->GetDescription()), MatchingProperty::description, toolIndex);
			auto menuSection = info->tool->MenuSection;
			if (menuSection >= 0 && menuSection < int32_t(sd.msections.size()))
			{
				matchProperty(toLower(BuildString(sd.msections[menuSection].name)), MatchingProperty::menuDescription, toolIndex);
			}
		}

		std::erase_if(matches, [](auto &item) {
			return !item;
		});
		std::sort(matches.begin(), matches.end());
		matchingTools.clear();
		std::transform(matches.begin(), matches.end(), std::back_inserter(matchingTools), [&](auto &match) {
			return &*tools[match->toolIndex];
		});
	}

	ElementSearch::DispositionFlags ElementSearch::GetDisposition() const
	{
		if (GetHost().GetTouchUi())
		{
			return DispositionFlags::cancelMissing;
		}
		if (!matchingTools.empty())
		{
			return DispositionFlags::none;
		}
		return DispositionFlags::okDisabled;
	}

	void ElementSearch::Ok()
	{
		if (!GetHost().GetTouchUi())
		{
			game.SelectTool(0, matchingTools[0]->tool.get());

		}
		Exit();
	}

	void ElementSearch::Gui()
	{
		auto &g = GetHost();
		int32_t toolButtonsPerRow = g.IfTouchUi(14, 8);
		constexpr int32_t maxRows = 12; // and an extra half row is shown to indicate that scrolling is possible

		auto title = g.IfTouchUi("DEFAULT_LANG_ELEMENTSEARCH_SELECT"_St, "DEFAULT_LANG_ELEMENTSEARCH_TITLE"_St)();
		auto elementSearch = ScopedDialog("elementSearch", title);

		BeginHPanel("searchBar");
		SetSize(Common{});
		SetSpacing(3);

		BeginTextbox("query", query, BuildString("[", "DEFAULT_LANG_ELEMENTSEARCH_QUERY"_St, "]"), TextboxFlags::none);
		SetMaxSizeSecondary(MaxSizeFitParent{});
		if (focusQuery)
		{
			GiveInputFocus();
			focusQuery = false;
		}
		if (EndTextbox())
		{
			Update();
		}
		BeginButton("setFavorite", Gui::iconStar, toggleFavorite ? ButtonFlags::stuck : ButtonFlags::none, 0xFFFFCC00_argb);
		SetSize(Common{});
		if (EndButton())
		{
			toggleFavorite = !toggleFavorite;
		}

		EndPanel();

		auto tools = ScopedScrollpanel("tools");
		auto panning = ScrollpanelIsPanning();
		SetAlignment(Gui::Alignment::top);
		SetMaxSizeSecondary(MaxSizeFitParent{});
		SetParentFillRatio(0);
		SetSize((game.toolTextureDataSize.Y + 2 * toolButtonPadding + toolButtonSpacing) * (2 * maxRows + 1) / 2 + toolButtonSpacing + 2);
		SetSizeSecondary((game.toolTextureDataSize.X + 2 * toolButtonPadding + toolButtonSpacing) * toolButtonsPerRow + toolButtonSpacing + 2);
		SetPadding(toolButtonPadding);
		SetSpacing(toolButtonSpacing);
		auto r = GetRect();
		lastHoveredTool = nullptr;
		int32_t buttonIndex = 0;
		int32_t rowIndex = 0;
		bool inHPanel = false;

		auto addTool = [&](const GameToolInfo &info, bool rightAlign)
		{
			if (!inHPanel)
			{
				BeginHPanel(rowIndex++);
				if (rightAlign)
				{
					SetAlignment(Gui::Alignment::right);
					SetOrder(Order::rightToLeft);
				}
				else
				{
					SetAlignment(Gui::Alignment::left);
				}

				inHPanel = true;
			}

			auto cell = ScopedComponent(buttonIndex++);
			SetSize(game.toolTextureDataSize.X + 2 * toolButtonPadding);
			if (game.GuiToolButton(*this, info, *toolAtlasTexture, true))
			{
				if (!panning)
				{
					// QueueToolTip(info->tool->Description.ToUtf8(), ...) // TODO-REDO_UI
					lastHoveredTool = info.tool.get();
				}
			}

			if (inHPanel && buttonIndex % toolButtonsPerRow == 0)
			{
				EndPanel();
				inHPanel = false;
			}
		};

		if (query.empty())
		{
			auto &sd = SimulationData::CRef();
			auto &tools = game.GetTools();

			static const std::array<std::string, 16> iconOverrides = {{ // TODO-REDO_UI: move this where menu sections are defined
				Gui::iconWalls,
				Gui::iconElectronic,
				Gui::iconPowered,
				Gui::iconSensor,
				Gui::iconForce,
				Gui::iconExplosive,
				Gui::iconGas,
				Gui::iconLiquid,
				Gui::iconPowder,
				Gui::iconSolid,
				Gui::iconRadioactive,
				Gui::iconStar,
				Gui::iconGol,
				Gui::iconTool,
				Gui::iconFavorite,
				Gui::iconDeco,
			}};
			Assert(sd.msections.size() == iconOverrides.size());

			auto addMenu = [&](int32_t i) {
				TextSeparator(rowIndex++, BuildString(" ", sd.msections[i].name, " ", iconOverrides[i], " "));

				buttonIndex = 0;
				for (auto &info : tools)
				{
						if (info && info->tool->MenuVisible && (info->tool->MenuSection == i || (i == SC_FAVORITES && game.favorites.contains(info->tool->Identifier))))
						{
								addTool(*info, true);
						}
				}

				if (inHPanel)
				{
						EndPanel();
						inHPanel = false;
				}
			};

			if (!game.favorites.empty())
			{
				addMenu(SC_FAVORITES);
			}
			for (int32_t i = 0; i < int32_t(sd.msections.size()); i++)
			{
				if (i != SC_FAVORITES)
				{
					addMenu(i);
				}
			}
		}
		else
		{
			while (buttonIndex < int32_t(matchingTools.size()))
			{
				addTool(*matchingTools[buttonIndex], false);
			}
		}

		if (inHPanel)
		{
			EndPanel();
		}

		g.DrawRect(r, 0xFFFFFFFF_argb);
	}

	bool ElementSearch::HandleEvent(const SDL_Event &event)
	{
		auto handledByView = View::HandleEvent(event);
		if (MayBeHandledExclusively(event) && handledByView && !lastHoveredTool)
		{
			return true;
		}
		auto handledByInputMapper = InputMapper::HandleEvent(event);
		if (MayBeHandledExclusively(event) && handledByInputMapper)
		{
			return true;
		}
		return false;
	}

	void ElementSearch::SetOnTop(bool newOnTop)
	{
		if (!newOnTop)
		{
			EndAllInputs();
		}
	}
}

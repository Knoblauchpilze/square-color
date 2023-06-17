
#include "Game.hh"
#include "Menu.hh"
#include <cxxabi.h>

namespace pge {
constexpr auto DEFAULT_BOARD_DIMS  = 32;
constexpr auto DEFAULT_MENU_HEIGHT = 50;

constexpr auto DEFAULT_GAME_FINISHED_ALERT_DURATION_IN_MS = 3000;

namespace {
auto generateMenu(const olc::vi2d &pos,
                  const olc::vi2d &size,
                  const std::string &text,
                  const std::string &name,
                  bool clickable     = false,
                  olc::Pixel bgColor = olc::VERY_DARK_GREEN) -> pge::MenuShPtr
{
  pge::menu::MenuContentDesc fd = pge::menu::newMenuContent(text, "", size);

  fd.color  = bgColor == olc::BLACK ? olc::WHITE : olc::BLACK;
  fd.hColor = bgColor == olc::BLACK ? olc::WHITE : olc::BLACK;

  fd.align = pge::menu::Alignment::Center;

  return std::make_shared<pge::Menu>(pos,
                                     size,
                                     name,
                                     pge::menu::newColoredBackground(bgColor),
                                     fd,
                                     pge::menu::Layout::Horizontal,
                                     clickable,
                                     false);
}

enum class MessageBoxKind
{
  Info,
  Warning,
  Alert
};

auto textColorFromMessageBoxKind(const MessageBoxKind kind) -> olc::Pixel
{
  switch (kind)
  {
    case MessageBoxKind::Info:
    default:
      return olc::VERY_DARK_GREEN;
    case MessageBoxKind::Warning:
      return olc::VERY_DARK_YELLOW;
    case MessageBoxKind::Alert:
      return olc::VERY_DARK_RED;
  }
}

auto backgroundColorFromMessageBoxKind(const MessageBoxKind kind) -> olc::Pixel
{
  switch (kind)
  {
    case MessageBoxKind::Info:
    default:
      return olc::GREEN;
    case MessageBoxKind::Warning:
      return olc::YELLOW;
    case MessageBoxKind::Alert:
      return olc::RED;
  }
}

auto generateMessageBoxMenu(const olc::vi2d &pos,
                            const olc::vi2d &size,
                            const std::string &text,
                            const std::string &name,
                            MessageBoxKind kind) -> pge::MenuShPtr
{
  pge::menu::MenuContentDesc fd = pge::menu::newMenuContent(text, "", size);
  fd.color                      = backgroundColorFromMessageBoxKind(kind);
  fd.align                      = pge::menu::Alignment::Center;

  return std::make_shared<pge::Menu>(pos,
                                     size,
                                     name,
                                     pge::menu::newColoredBackground(
                                       textColorFromMessageBoxKind(kind)),
                                     fd,
                                     pge::menu::Layout::Horizontal,
                                     false,
                                     false);
}
} // namespace

Game::Game()
  : utils::CoreObject("game")
  , m_state(State{
      true,  // paused
      true,  // disabled
      false, // terminated
      Color::White,
      Color::Black,
    })
  , m_menus()
  , m_board(std::make_shared<Board>(DEFAULT_BOARD_DIMS, DEFAULT_BOARD_DIMS))
{
  setService("game");

  m_state.playerColor = m_board->colorOf(Owner::Player);
  m_state.aiColor     = m_board->colorOf(Owner::AI);
}

Game::~Game() {}

std::vector<MenuShPtr> Game::generateMenus(float width, float height)
{
  std::vector<MenuShPtr> out;

  auto menus = generateTerritoryMenu(width, height);
  for (auto &menu : menus)
  {
    out.push_back(menu);
  }

  menus = generateColorButtons(width, height);
  for (auto &menu : menus)
  {
    out.push_back(menu);
  }

  menus = generateGameOver(width, height);
  for (auto &menu : menus)
  {
    out.push_back(menu);
  }

  return out;
}

void Game::performAction(float /*x*/, float /*y*/)
{
  // Only handle actions when the game is not disabled.
  if (m_state.disabled)
  {
    log("Ignoring action while menu is disabled");
    return;
  }
}

bool Game::step(float /*tDelta*/)
{
  // When the game is paused it is not over yet.
  if (m_state.paused)
  {
    return true;
  }

  updateUI();

  auto done = (m_board->status() != Status::Running && !m_menus.win.menu->visible()
               && !m_menus.draw.menu->visible() && !m_menus.lost.menu->visible());
  if (done)
  {
    pause();
    enable(!m_state.paused);
  }

  return !done;
}

void Game::togglePause()
{
  if (m_state.paused)
  {
    resume();
  }
  else
  {
    pause();
  }

  enable(!m_state.paused);
}

const Board &Game::board() const noexcept
{
  return *m_board;
}

void Game::setPlayerColor(const Color &color)
{
  if (m_state.playerColor == color)
  {
    warn("ignoring change to color " + colorName(color), "player already has this color");
    return;
  }

  m_menus.colors[m_state.playerColor]->setEnabled(true);
  m_menus.colors[m_state.aiColor]->setEnabled(true);

  m_board->changeColorOf(Owner::Player, color);
  const auto aiColor = m_board->bestColorFor(Owner::AI);
  m_board->changeColorOf(Owner::AI, aiColor);

  m_menus.colors[color]->setEnabled(false);
  if (m_board->isPlayerAndAiInContact())
  {
    m_menus.colors[m_board->colorOf(Owner::AI)]->setEnabled(false);
  }

  m_state.playerColor = color;
  m_state.aiColor     = aiColor;
  info("player now has color " + colorName(color));
  info("ai choses " + colorName(aiColor));
}

void Game::save(const std::string &file) const noexcept
{
  m_board->save(file);
}

void Game::load(const std::string &file)
{
  m_board->load(file);
  updateUIAfterBoardChange();
}

void Game::reset()
{
  log("Reset board");
  m_board = std::make_shared<Board>(DEFAULT_BOARD_DIMS, DEFAULT_BOARD_DIMS);
  updateUIAfterBoardChange();
}

void Game::enable(bool enable)
{
  m_state.disabled = !enable;

  if (m_state.disabled)
  {
    log("Disabled game UI", utils::Level::Verbose);
  }
  else
  {
    log("Enabled game UI", utils::Level::Verbose);
  }
}

void Game::updateUI()
{
  const auto writeTerritory = [](float perc, const std::string &owner) {
    std::stringstream ss;
    ss.precision(2);
    ss << perc * 100.0f;
    return owner + ": " + ss.str() + "%";
  };

  auto str = writeTerritory(m_board->occupiedBy(Owner::Player), "player");
  m_menus.playerTerritory->setText(str);

  str = writeTerritory(m_board->occupiedBy(Owner::AI), "ai");
  m_menus.aiTerritory->setText(str);

  m_menus.win.update(m_board->status() == Status::Win);
  m_menus.draw.update(m_board->status() == Status::Draw);
  m_menus.lost.update(m_board->status() == Status::Lost);
}

auto Game::generateTerritoryMenu(int width, int /*height*/) -> std::vector<MenuShPtr>
{
  m_menus.playerTerritory = generateMenu(olc::vi2d{},
                                         olc::vi2d{width, DEFAULT_MENU_HEIGHT},
                                         "player: 0%",
                                         "player_territory",
                                         false,
                                         olc::GREEN);
  m_menus.aiTerritory     = generateMenu(olc::vi2d{},
                                     olc::vi2d{width, DEFAULT_MENU_HEIGHT},
                                     "ai: 0%",
                                     "ai_territory",
                                     false,
                                     olc::DARK_GREEN);

  auto top = generateMenu(olc::vi2d{},
                          olc::vi2d{width, DEFAULT_MENU_HEIGHT},
                          "",
                          "territories",
                          false);
  top->addMenu(m_menus.playerTerritory);
  top->addMenu(m_menus.aiTerritory);

  return {top};
}

auto Game::generateColorButtons(int width, int height) -> std::vector<MenuShPtr>
{
  std::vector<MenuShPtr> out{};
  auto colors = generateMenu(olc::vi2d{0, height - DEFAULT_MENU_HEIGHT},
                             olc::vi2d{width, DEFAULT_MENU_HEIGHT},
                             "",
                             "colors",
                             false);

  for (int cId = 0; cId < static_cast<int>(Color::Count); ++cId)
  {
    const auto c = static_cast<Color>(cId);

    auto color = generateMenu(olc::vi2d{},
                              olc::vi2d{10, DEFAULT_MENU_HEIGHT},
                              colorName(c),
                              "color",
                              true,
                              olcColorFromCellColor(c));
    color->setSimpleAction([c](Game &g) { g.setPlayerColor(c); });
    color->setEnabled(c != m_state.playerColor);

    m_menus.colors[c] = color;

    colors->addMenu(color);
  }

  out.push_back(colors);
  return out;
}

auto Game::generateGameOver(int width, int height) -> std::vector<MenuShPtr>
{
  m_menus.win.date      = utils::TimeStamp();
  m_menus.win.wasActive = false;
  m_menus.win.duration  = DEFAULT_GAME_FINISHED_ALERT_DURATION_IN_MS;
  m_menus.win.menu      = generateMessageBoxMenu(olc::vi2d((width - 300.0f) / 2.0f,
                                                      (height - 150.0f) / 2.0f),
                                            olc::vi2d(300, 150),
                                            "You won !",
                                            "win",
                                            MessageBoxKind::Info);
  m_menus.win.menu->setVisible(false);

  m_menus.draw.date      = utils::TimeStamp();
  m_menus.draw.wasActive = false;
  m_menus.draw.duration  = DEFAULT_GAME_FINISHED_ALERT_DURATION_IN_MS;
  m_menus.draw.menu      = generateMessageBoxMenu(olc::vi2d((width - 300.0f) / 2.0f,
                                                       (height - 150.0f) / 2.0f),
                                             olc::vi2d(300, 150),
                                             "It's a draw !",
                                             "draw",
                                             MessageBoxKind::Warning);
  m_menus.draw.menu->setVisible(false);

  m_menus.lost.date      = utils::TimeStamp();
  m_menus.lost.wasActive = false;
  m_menus.lost.duration  = DEFAULT_GAME_FINISHED_ALERT_DURATION_IN_MS;
  m_menus.lost.menu      = generateMessageBoxMenu(olc::vi2d((width - 300.0f) / 2.0f,
                                                       (height - 150.0f) / 2.0f),
                                             olc::vi2d(300, 150),
                                             "You lost !",
                                             "lost",
                                             MessageBoxKind::Alert);
  m_menus.lost.menu->setVisible(false);

  std::vector<MenuShPtr> out;
  out.push_back(m_menus.win.menu);
  out.push_back(m_menus.draw.menu);
  out.push_back(m_menus.lost.menu);
  return out;
}

void Game::updateUIAfterBoardChange() noexcept
{
  m_state.playerColor = m_board->colorOf(Owner::Player);
  m_state.aiColor     = m_board->colorOf(Owner::AI);

  for (auto &[color, menu] : m_menus.colors)
  {
    menu->setEnabled(color != m_state.playerColor);
  }

  if (m_board->isPlayerAndAiInContact())
  {
    m_menus.colors[m_board->colorOf(Owner::AI)]->setEnabled(false);
  }
}

bool Game::TimedMenu::update(bool active) noexcept
{
  // In case the menu should be active.
  if (active)
  {
    if (!wasActive)
    {
      // Make it active if it's the first time that
      // we detect that it should be active.
      date      = utils::now();
      wasActive = true;
      menu->setVisible(true);
    }
    else if (utils::now() > date + utils::toMilliseconds(duration))
    {
      // Deactivate the menu in case it's been active
      // for too long.
      menu->setVisible(false);
    }
    else
    {
      // Update the alpha value in case it's active
      // for not long enough.
      olc::Pixel c = menu->getBackgroundColor();

      float d = utils::diffInMs(date, utils::now()) / duration;
      c.a     = static_cast<uint8_t>(std::clamp((1.0f - d) * pge::alpha::Opaque, 0.0f, 255.0f));
      menu->setBackground(pge::menu::newColoredBackground(c));
    }
  }
  // Or if the menu shouldn't be active anymore and
  // it's the first time we detect that.
  else if (wasActive)
  {
    // Deactivate the menu.
    menu->setVisible(false);
    wasActive = false;
  }

  return menu->visible();
}

} // namespace pge

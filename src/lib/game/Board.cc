
#include "Board.hh"
#include <fstream>
#include <unordered_set>
#include <unordered_map>

namespace pge {

Board::Board(int width, int height)
  : utils::CoreObject("board")
  , m_width(width)
  , m_height(height)
{
  setService("square");
  if (m_width <= 0 || m_height <= 0)
  {
    error("Failed to initialize board",
          "Invalid dimensions " + std::to_string(m_width) + "x" + std::to_string(m_height));
  }

  initialize();
}

int Board::width() const noexcept
{
  return m_width;
}

int Board::height() const noexcept
{
  return m_height;
}

Cell Board::at(int x, int y) const
{
  if (x < 0 || x >= m_width || y < 0 || y >= m_height)
  {
    error("Failed to get cell",
          "Invalid coordinates " + std::to_string(x) + "x" + std::to_string(y));
  }

  return m_cells[linear(x, y)];
}

auto Board::colorOf(const Owner &owner) const noexcept -> Color
{
  switch (owner)
  {
    case Owner::Player:
      return at(0, 0).color;
    case Owner::AI:
      return at(width() - 1, height() - 1).color;
    default:
      error("Can't determine color", "Invalid owner " + std::to_string(static_cast<int>(owner)));
  }

  // Not reachable.
  return Color::Blue;
}

bool Board::isPlayerAndAiInContact() const noexcept
{
  for (auto y = 0; y < height(); ++y)
  {
    for (auto x = 0; x < width(); ++x)
    {
      const auto c = m_cells[linear(x, y)];
      if (c.owner != Owner::Player)
      {
        continue;
      }

      if (hasBorderWith(x, y, Owner::AI))
      {
        return true;
      }
    }
  }

  return false;
}

float Board::occupiedBy(const Owner &owner) const noexcept
{
  return 1.0f * countFor(owner) / m_cells.size();
}

void Board::changeColorOf(const Owner &owner, const Color &color) noexcept
{
  std::for_each(m_cells.begin(), m_cells.end(), [&owner, &color](Cell &c) {
    if (c.owner == owner)
    {
      c.color = color;
    }
  });

  auto gained = 0;
  for (auto y = 0; y < height(); ++y)
  {
    for (auto x = 0; x < width(); ++x)
    {
      auto &c = m_cells[linear(x, y)];
      if (c.owner == Owner::Nobody && c.color == color && hasBorderWith(x, y, owner))
      {
        c.owner = owner;
        ++gained;
      }
    }
  }

  log(ownerName(owner) + " gained " + std::to_string(gained) + " cell(s)");
  updateStatus();
}

auto Board::bestColorFor(const Owner &owner) const noexcept -> Color
{
  struct Gain
  {
    Color color;
    int amount;
  };
  std::vector<Gain> gainPerColor(static_cast<int>(Color::Count));
  const auto otherColor = colorOf(owner == Owner::AI ? Owner::Player : Owner::AI);
  const auto areInContact = isPlayerAndAiInContact();

  std::unordered_set<std::string> usedGains;
  bool zeroGain = true;
  const auto validCell = [this](const int x, const int y) {
    return x >= 0 && y >= 0 && x < m_width && y < m_height;
  };
  const auto toKey = [](const int x, const int y) {
    return std::to_string(x) + "x" + std::to_string(y);
  };
  const auto isGainForColor = [&usedGains, &toKey, this](const int x, const int y, const Color &c) {
    if (usedGains.count(toKey(x, y)) > 0)
    {
      return false;
    }

    return m_cells[linear(x, y)].owner == Owner::Nobody && m_cells[linear(x, y)].color == c;
  };

  const auto accumulateGain =
    [&validCell, &isGainForColor, &gainPerColor, &usedGains, &toKey, this](const int x,
                                                                           const int y,
                                                                           const int cId) {
      if (validCell(x, y) && isGainForColor(x, y, static_cast<Color>(cId)))
      {
        ++gainPerColor[cId].amount;
      }
      usedGains.insert(toKey(x, y));
    };

  for (auto cId = 0u; cId < gainPerColor.size(); ++cId)
  {
    gainPerColor[cId].color  = static_cast<Color>(cId);
    gainPerColor[cId].amount = 0;

    if (gainPerColor[cId].color == otherColor && areInContact) {
      log("Ignoring " + colorName(gainPerColor[cId].color) + ", opponent has this color");
      continue;
    }

    for (auto y = 0; y < m_height; ++y)
    {
      for (auto x = 0; x < m_width; ++x)
      {
        const auto &cell = m_cells[linear(x, y)];

        if (cell.owner != owner)
        {
          continue;
        }

        accumulateGain(x, y + 1, cId);
        accumulateGain(x, y - 1, cId);
        accumulateGain(x - 1, y, cId);
        accumulateGain(x + 1, y, cId);
      }
    }

    log("Gain for " + colorName(gainPerColor[cId].color) + " is "
        + std::to_string(gainPerColor[cId].amount));
    usedGains.clear();
    zeroGain &= (gainPerColor[cId].amount == 0);
  }

  std::sort(gainPerColor.begin(), gainPerColor.end(), [](const Gain &lhs, const Gain &rhs) {
    return lhs.amount > rhs.amount;
  });

  if (zeroGain) {
    // Random color.
    Color pick = otherColor;
    constexpr auto TRIES_FOR_RANDOM_COLOR = static_cast<int>(Color::Count);
    auto tries = 0;
    while (pick == otherColor && tries < TRIES_FOR_RANDOM_COLOR) {
      pick = static_cast<Color>(std::rand() % static_cast<int>(Color::Count));
      ++tries;
    }

    if (pick == otherColor) {
      warn("Failed to pick a random color, continuing with first one");
    }
    else {
      return pick;
    }
  }

  return gainPerColor.front().color;
}

auto Board::status() const noexcept -> Status
{
  return m_status;
}

void Board::save(const std::string &file) const noexcept
{
  std::ofstream out(file.c_str());
  if (!out.good())
  {
    error("Failed to save board to \"" + file + "\"", "Failed to open file");
  }

  unsigned buf, size = sizeof(unsigned);
  const char *raw = reinterpret_cast<const char *>(&buf);

  buf = m_width;
  out.write(raw, size);

  buf = m_height;
  out.write(raw, size);

  for (const auto &c : m_cells)
  {
    buf = static_cast<unsigned>(c.owner);
    out.write(raw, size);

    buf = static_cast<unsigned>(c.color);
    out.write(raw, size);
  }

  log("Saved content of board with dimensions " + std::to_string(m_width) + "x"
        + std::to_string(m_height) + " to \"" + file + "\"",
      utils::Level::Info);
}

void Board::load(const std::string &file)
{
  std::ifstream out(file.c_str());
  if (!out.good())
  {
    error("Failed to load board to \"" + file + "\"", "Failed to open file");
  }

  out.read(reinterpret_cast<char *>(&m_width), sizeof(unsigned));
  out.read(reinterpret_cast<char *>(&m_height), sizeof(unsigned));

  if (m_width == 0u || m_height == 0u)
  {
    error("Failed to load board from file \"" + file + "\"",
          "Invalid board of size " + std::to_string(m_width) + "x" + std::to_string(m_height));
  }

  m_cells.resize(m_width * m_height);

  unsigned buf, size = sizeof(unsigned);
  char *raw = reinterpret_cast<char *>(&buf);
  for (unsigned id = 0u; id < m_cells.size(); ++id)
  {
    auto &c = m_cells[id];

    out.read(raw, size);
    c.owner = static_cast<Owner>(buf);

    out.read(raw, size);
    c.color = static_cast<Color>(buf);
  }

  updateStatus();

  log("Loaded board with dimensions " + std::to_string(m_width) + "x" + std::to_string(m_height),
      utils::Level::Info);
}

void Board::initialize()
{
  m_cells.resize(m_width * m_height);

  for (auto &cell : m_cells)
  {
    cell.color = generateRandomColor();
  }

  auto &player          = m_cells[linear(0, 0)];
  player.owner          = Owner::Player;
  m_cells[linear(1, 0)] = player;
  m_cells[linear(1, 1)] = player;
  m_cells[linear(0, 1)] = player;

  auto &ai = m_cells[linear(width() - 1, height() - 1)];
  ai.owner = Owner::AI;
  while (player.color == ai.color)
  {
    ai.color = generateRandomColor();
  }
  m_cells[linear(width() - 1, height() - 2)] = ai;
  m_cells[linear(width() - 2, height() - 2)] = ai;
  m_cells[linear(width() - 2, height() - 1)] = ai;
}

int Board::linear(int x, int y) const noexcept
{
  return y * width() + x;
}

bool Board::hasBorderWith(int x, int y, const Owner &owner) const noexcept
{
  const auto w         = width();
  const auto h         = height();
  const auto validCell = [&w, &h](const int x, const int y) {
    return x >= 0 && y >= 0 && x < w && y < h;
  };

  const auto top    = linear(x, y + 1);
  const auto bottom = linear(x, y - 1);
  const auto left   = linear(x - 1, y);
  const auto right  = linear(x + 1, y);

  if (validCell(x, y + 1) && m_cells[top].owner == owner)
  {
    return true;
  }
  if (validCell(x, y - 1) && m_cells[bottom].owner == owner)
  {
    return true;
  }
  if (validCell(x - 1, y) && m_cells[left].owner == owner)
  {
    return true;
  }
  if (validCell(x + 1, y) && m_cells[right].owner == owner)
  {
    return true;
  }

  return false;
}

auto Board::countFor(const Owner &owner) const noexcept -> int
{
  return static_cast<int>(
    std::count_if(m_cells.begin(),
                         m_cells.end(),
                         [&owner](const Cell &c) { return c.owner == owner; }));
}

void Board::updateStatus() noexcept
{
  std::unordered_map<Owner, int> cellsToGain;

  const auto validCell = [this](const int x, const int y) {
    return x >= 0 && y >= 0 && x < m_width && y < m_height;
  };

  bool someCellsToGain = false;
  auto y = 0;
  while (y < m_height && !someCellsToGain) {
    auto x = 0;

    while (x < m_width && !someCellsToGain) {
      const auto& c = m_cells[linear(x, y)];
      if (c.owner == Owner::Nobody) {
        if (validCell(x + 1, y)) {
          const auto& right = m_cells[linear(x + 1, y)];
          someCellsToGain |= (right.owner != Owner::Nobody);
        }
        if (validCell(x - 1, y)) {
          const auto& left = m_cells[linear(x - 1, y)];
          someCellsToGain |= (left.owner != Owner::Nobody);
        }
        if (validCell(x, y + 1)) {
          const auto& top = m_cells[linear(x, y + 1)];
          someCellsToGain |= (top.owner != Owner::Nobody);
        }
        if (validCell(x, y - 1)) {
          const auto& bottom = m_cells[linear(x, y - 1)];
          someCellsToGain |= (bottom.owner != Owner::Nobody);
        }
      }

      ++x;
    }

    ++y;
  }

  auto player = countFor(Owner::Player);
  auto ai = countFor(Owner::AI);

  if (!someCellsToGain) {
    info("player: " + std::to_string(player) + " - ai: " + std::to_string(ai));
  }

  if (someCellsToGain) {
    m_status = Status::Running;
  }
  else if (player == ai) {
    m_status = Status::Draw;
  }
  else if (player > ai) {
    m_status = Status::Win;
  }
  else {
    m_status = Status::Lost;
  }
}

Color generateRandomColor() noexcept
{
  const auto max   = static_cast<int>(Color::Count);
  const auto index = std::rand() % max;

  return static_cast<Color>(index);
}

auto olcColorFromCellColor(const Color &c) -> olc::Pixel
{
  switch (c)
  {
    case Color::Red:
      return olc::RED;
    case Color::Green:
      return olc::GREEN;
    case Color::Blue:
      return olc::BLUE;
    case Color::Yellow:
      return olc::YELLOW;
    case Color::Cyan:
      return olc::CYAN;
    case Color::Magenta:
      return olc::MAGENTA;
    case Color::Black:
      return olc::BLACK;
    case Color::White:
      return olc::WHITE;
    default:
      return olc::GREY;
  }
}

auto colorName(const Color &c) -> std::string
{
  switch (c)
  {
    case Color::Red:
      return "red";
    case Color::Green:
      return "green";
    case Color::Blue:
      return "blue";
    case Color::Yellow:
      return "yellow";
    case Color::Cyan:
      return "cyan";
    case Color::Magenta:
      return "magenta";
    case Color::Black:
      return "black";
    case Color::White:
      return "white";
    default:
      return "unknown";
  }
}

auto ownerName(const Owner& o) -> std::string
{
  switch (o)
  {
    case Owner::Nobody:
      return "nobody";
    case Owner::AI:
      return "ai";
    case Owner::Player:
      return "player";
    default:
      return "unknown";
  }
}

} // namespace pge
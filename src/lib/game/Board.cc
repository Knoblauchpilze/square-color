
#include "Board.hh"

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

Color Board::playerColor() const noexcept
{
  return at(0, 0).color;
}

float Board::occupiedBy(const Owner &owner) const noexcept
{
  return 1.0f
         * std::count_if(m_cells.begin(),
                         m_cells.end(),
                         [&owner](const Cell &c) { return c.owner == owner; })
         / m_cells.size();
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

} // namespace pge
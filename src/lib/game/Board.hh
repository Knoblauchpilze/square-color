
#pragma once

#include <core_utils/CoreObject.hh>
#include <memory>
#include <olcEngine.hh>

namespace pge {

/// @brief - Who owns a tile.
enum class Owner
{
  Nobody,
  AI,
  Player
};

/// @brief - The available colors for a cell.
enum class Color
{
  Red,
  Green,
  Blue,
  Yellow,
  Cyan,
  Magenta,
  Black,
  White,
  Count
};

/// @brief - A cell and its properties.
struct Cell
{
  Owner owner{Owner::Nobody};
  Color color{Color::Black};
};

/// @brief - The state of the board.
enum class Status
{
  Running,
  Win,
  Draw,
  Lost
};

/// @brief - The board, regrouping a certain amount of cells.
class Board : public utils::CoreObject
{
  public:
  Board(int width, int height);

  int width() const noexcept;

  int height() const noexcept;

  Cell at(int x, int y) const;

  Color playerColor() const noexcept;

  Color aiColor() const noexcept;

  bool isPlayerAndAiInContact() const noexcept;

  float occupiedBy(const Owner &owner) const noexcept;

  void changeColorOf(const Owner &owner, const Color &color) noexcept;

  auto status() const noexcept -> Status;

  void save(const std::string &file) const noexcept;
  void load(const std::string &file);

  private:
  int m_width;
  int m_height;

  std::vector<Cell> m_cells{};

  Status m_status{Status::Running};

  void initialize();
  int linear(int x, int y) const noexcept;
  bool hasBorderWith(int x, int y, const Owner &owner) const noexcept;
  void updateStatus() noexcept;
};

using BoardShPtr = std::shared_ptr<Board>;

Color generateRandomColor() noexcept;
auto olcColorFromCellColor(const Color &c) -> olc::Pixel;
auto colorName(const Color &c) -> std::string;
} // namespace pge

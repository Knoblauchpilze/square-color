
# pge-app

Simple implementation of an application relying on the [PixelGameEngine](https://github.com/OneLoneCoder/olcPixelGameEngine) to perform the rendering. This project comes with a basic event handling system, some basic menus which can register actions and a default `game` structure which can be extended to handle various processes.

It is meant as a simple way to build an application without having to reinvent the wheel for every project. Most of the behaviors can be customized in order to handle more complex behaviors (resources loading, step function, pause system, etc.).

The general architecture of the repository has been inspired by the one described [here](https://raymii.org/s/tutorials/Cpp_project_setup_with_cmake_and_unit_tests.html): this covers how to put organize the sources, the headers and the tests.

# Installation

## Prerequisite

This projects uses:
* [google test](https://github.com/google/googletest): installation instructions [here](https://www.eriksmistad.no/getting-started-with-google-test-on-ubuntu/), a simple `apt-get` should be enough.
* `cmake`: installation instructions [here](https://askubuntu.com/questions/355565/how-do-i-install-the-latest-version-of-cmake-from-the-command-line), a simple `apt-get` should also be enough.
* [eigen](https://eigen.tuxfamily.org/index.php?title=Main_Page): installation instructions [here](https://www.cyberithub.com/how-to-install-eigen3-on-ubuntu-20-04-lts-focal-fossa/) for Ubuntu 20.04, a simple `sudo apt install libeigen3-dev` should be enough.

## Instructions

* Clone the repo: `git clone git@github.com:Knoblauchpilze/pge-app.git`.
* Clone dependencies:
    * [core_utils](https://github.com/Knoblauchpilze/core_utils)
    * [maths_utils](https://github.com/Knoblauchpilze/maths_utils)
* Go to the project's directory `cd ~/path/to/the/repo`.
* Compile: `make run`.

Don't forget to add `/usr/local/lib` to your `LD_LIBRARY_PATH` to be able to load shared libraries at runtime. This is handled automatically when using the `make run` target (which internally uses the [run.sh](data/run.sh) script).

## Configuration

If the user wants to update the project to another name and start with a more complex app, a convenience script is [provided](configureProject.sh) which allows to perform the renaming of elements as needed to have a new app with a different name.

The usage of the script is as follows:
```bash
./configureProject.sh project_name
```

# Generalities

The application is structured around a base [App](src/lib/App.hh) which can be customized to include more complex behaviors.

## Layers

The rendering is dividied into four layers:
* the debug layer is meant to handle every debug display and can be hidden for increased performance.
* the ui layer receives all the UI information such as menus. Can also be hidden if needed.
* the rendering layer receives any resource that compose the main content of the application.
* the decal layer is meant to receive any GPU accelerated resource.

The ordering of the layer matters as it will describe how elements are overlaid. The order is as follows:
* decal layer
* non-decal graphical resource layer
* ui layer
* debug layer

When pressing the `D` key the debug layer can easily be toggled on or off.

## Game

The application provides a base [Game](src/lib/game/Game.hh) class which can be used to wrap the application's data into a structure that can communicate easily with the application. Some general methods have been extracted to provide hooks that are used by the default application to make the game evolve.

While this class is called `Game` it can also receive some other type of data.

Some generic types are also provided to help easily create some menus and UI elements for a game. See more details in the [specialization](#specializing-the-project) section.

## App

The [App](src/lib/App.hh) class is the element which is executed by default when starting the project. It aims at receiving the logic to control the interaction with the user, the rendering and the controlling of the game/app elements.

Multiple hooks are provided where the user can insert custom behavior in order to enrich the base implementation with new processes and features.

## In-app content

The base project comes with a set of basic features to start developing either a game or an app. Some of them are detailed below.

### Creating an App

In order to ease the creation of an `App` object, we regrouped the options within a struct named [AppDesc](src/lib/app/AppDesc.hh). This contains several attributes, among which:
* the dimensions of the window in pixels.
* a coordinate frame (see more details in the dedicated [section](#coordinate-frame)).
* whether or not the view allows panning and zooming.

### Coordinate frame

Usually, an application maps what happens on the screen to an internal coordinate frame. The start of most interactions begins in pixels frame: the user clicks somewhere or moves the mouse somewhere and we get the information about where the mouse is in pixels. From this, the application needs to transform these coordinates and see if anything interesting happens because of this.

The base app defines a coordinate frame to handle such things. A [coordinate frame](src/lib/coordinates/CoordinateFrame.hh) is an interface which aims at converting the information in pixels space to the internal world space. To do this, it uses two [viewports](src/lib/coordinates/Viewport.hh): one defining the pixels space and the other one the tiles space.

By default, two separate frames are already available:
* a [top view](src/lib/coordinates/TopViewFrame.hh) frame: this represents a simple scaling between the position in pixels and the position in tiles. It can be used for a top down app, or most 2D apps.
* an [isometric view](src/lib/coordinates/IsometricViewFrame.hh) frame: this represents a semi-3D projection which allows to produce graphics like [this](https://en.wikipedia.org/wiki/Isometric_video_game_graphics).

Panning and zooming are handled for both frames, along with converting from pixels to tiles and vice versa.

### Fixing the frame

For some applications, it might be undesirable to have panning and zooming enabled (for example a sudoku app). The `AppDesc` structure allows to configure this through the `fixedFrame` boolean which prevents any panning and zooming to be considered. The application will be blocked on the tiles defined in the main viewport provided when creating the application.

### Logging

By default the app comes with a logger allowing to print some messages in the console executing the program. Most of the objects provided in the app are also integrating a logger which makes it possible to debug the processes easily.

In case the user wants to access more log messages or reduce the severity of logs produced by the app, it is easy to adjust the `raw.setLevel` call to not use `Debug` but another level.

### Putting it together

This is the default `main` program provided in this template app:

```cpp
int
main(int /*argc*/, char** /*argv*/) {
  // Create the logger.
  utils::StdLogger raw;
  raw.setLevel(utils::Level::Debug);
  utils::PrefixedLogger logger("pge", "main");
  utils::LoggerLocator::provide(&raw);

  logger.logMessage(utils::Level::Notice, "Starting application");

  /// FIXME: Definition of the viewports: the tiles viewport and the pixels viewport.
  auto tiles  = pge::CenteredViewport({0.0f, 0.0f}, {4.0f, 3.0f});
  auto pixels = pge::TopLeftViewport({0.0f, 0.0f}, {800.0f, 600.0f});

  pge::CoordinateFramePtr frame;
  auto useIsometric = true;
  if (useIsometric)
  {
    frame = std::make_shared<pge::IsometricViewFrame>(tiles, pixels);
  }
  else
  {
    frame = std::make_shared<pge::TopViewFrame>(tiles, pixels);
  }

  pge::AppDesc ad = pge::newDesc(olc::vi2d(800, 600), frame, "pge-app");
  pge::App demo(ad);

  demo.Start();

  return EXIT_SUCCESS;
}
```

Both the tiles and pixels viewports are important and define respectively how much of the world will be visible and how zoomed-in the initial setup will be. Also, the user can configure whether an isometric view frame should be used or rather a top view one.

# Profiling

A convenience script is provided in order to profile the app. This comes from [this](https://stackoverflow.com/a/771005) answer (although the other answers are interesting as well). This requires a few things to be installed on the system:
* [GIMP](https://doc.ubuntu-fr.org/gimp)
* [valgrind](https://wiki.ubuntu.com/Valgrind)
* [gprof2dot](https://github.com/jrfonseca/gprof2dot)

The output image is a png that is opened with GIMP and can give ideas about what is slowing down the application.

The profiling can be triggered with the following command:
```bash
make profile
```

# Testing

## Generalities

The default application comes with a functional testing framework. The tests are meant to be located in the [tests](tests/) folder and can be further specialized into unit tests (existing [here](tests/unit) already) but also integration tests, functional tests, etc.

The framework uses the `gtest` library to perform the testing.

## Adding tests

In order to add a new test, one can create a new file under the relevant test section (say `tests/unit/lib/MyClassTest.cc`). The structure of the file should look something like so:
```cpp

# include "MyClass.hh"
# include <gtest/gtest.h>

using namespace ::testing;

namespace the::namespace::of::the::class {

TEST(Unit_MyClass, Test_MyFeature)
{
  /* FIXME: Some testing. */
  EXPECT_EQ(/* Some condition */);
}

}
```

## Run the tests

Once the tests are written, the root `Makefile` defines a target to execute all the tests under:
```bash
make tests
```

In case the test suite is growing one can add some targets to run only the unit or integration tests but it is not provided yet.

# Specializing the project

## Generalities

The application and the class within it are designed to easily be reused and extended with various behaviors.

The classes which should be changed by the user are mainly:
* [App](src/lib/App.hh) class, described [here](#the-app-class).
* [Game](src/lib/game/Game.hh) class, described [here](#the-game-class).
* [GameState](src/lib/game/GameState.hh) class, described [here](#the-gamestate-class).

## The App class

The `App` class provides various methods which can be enriched with behaviors. Below is a description of the main hooks which can be specialized.

### loadResources

The `loadResources` method can be used to load the graphic resources needed by the `App`. This includes textures, sprites or any sort of graphic elements. A typical example could look like this:

```cpp
void
App::loadResources() {
  // Create the texture pack.
  sprites::Pack pack;
  pack.file = "data/img/pieces.png";
  const auto TILE_SIZE_IN_PIXELS = 64;
  pack.sSize = olc::vi2d(TILE_SIZE_IN_PIXELS, TILE_SIZE_IN_PIXELS);
  pack.layout = olc::vi2d(6, 2);

  m_piecesPackID = m_packs->registerPack(pack);
}
```
The `m_piecesPackID` defines an identifier which can then be used to reference a textures pack during the rendering phase (see the [drawDecal](#drawDecal)) section.

### loadMenuResources

The default implementation looks like so:
```cpp
void
App::loadMenuResources() {
  // Generate the game state.
  m_state = std::make_shared<GameState>(
    olc::vi2d(ScreenWidth(), ScreenHeight()),
    Screen::Home
  );

  m_menus = m_game->generateMenus(ScreenWidth(), ScreenHeight());
}
```

The main goal here is to create the `GameState` and the UI menus. When debugging the application or during the development process, it might be useful to change the `Screen::Home` statement to `Screen::Game` for example to avoid having to select each time a new game.

This method also handles the generation of menus and their registration in the main input loop: the `m_menus` attribute provided by the `App` class registers all menus and will call them whenever there's a change in the inputs from the user (see the [game](#the-game-class) section).

### drawDecal

Along with the `draw`, `drawUI` and `drawDebug` method the `drawDecal` allows to render the elements of the application on screen. The code for all of these methods is similar so let's see what it does:
```cpp
void
App::drawDecal(const RenderDesc& /*res*/) {
  // Clear rendering target.
  SetPixelMode(olc::Pixel::ALPHA);
  Clear(olc::VERY_DARK_GREY);

  // In case we're not in the game screen, do nothing.
  if (m_state->getScreen() != Screen::Game) {
    SetPixelMode(olc::Pixel::NORMAL);
    return;
  }

  /// FIXME: Add rendering code here.

  SetPixelMode(olc::Pixel::NORMAL);
}
```

The first thing to notice is that no matter what, the layer is cleared on each call. In case the screen is different from the `Game` screen (so basically when the main purpose of the application is displayed on screen), we don't do anything. This is to allow the other screens to draw what they want: usually this can be a UI or some configuration info.

The `drawDecal` method should be the preferred way to render complex elements as opposed to the `draw` method: using the `Decal` mechanism provided by the `Pixel Game Engine` we are able to render very quickly a lot of elements without impacting the framerate too much by leveraging the GPU.

The method is already set up so that the user can just insert code in the `FIXME` statement (which doesn't exist in the code). It is recommended to use new methods like `drawBoard`, `drawElements` etc. and perform the rendering there. This cleanly separates the steps to draw the elements. This is done in the demo app with the `renderDefaultTexturePack` method.

Whenever the user needs to perform the rendering of sprites, we define a convenience structure to help with drawing that:

```cpp
namespace sprites {
  struct Sprite {
    // The `pack` defines the identifier of the pack from
    // which the sprite should be picked.
    PackId pack;

    // The `sprite` defines an identifier for the sprite. The
    // position of the sprite in the resource pack will be
    // computed from this identifier.
    olc::vi2d sprite;

    // The `id` allows to select a variant for the sprite. By
    // default this value is `0` meaning the principal display
    // for the sprite.
    int id{0};

    // The `tint` defines a color to apply to tint the sprite
    // as a whole. Can also be used to provide some alpha.
    olc::Pixel tint{olc::WHITE};
  };
}

struct SpriteDesc {
  // The x coordinate of the sprite.
  float x;

  // The y coordinate of the sprite.
  float y;

  // The radius of the sprite: applied both along the x and y coordinates.
  float radius;

  // A description of the sprite.
  sprites::Sprite sprite;
};
```

These structures help encapsulate the logic to find and draw a sprite at a specific location on screen. An example code is provided below:

```cpp
void
App::drawFlowers(const RenderDesc& res) noexcept {
  SpriteDesc sd = {};
  sd.radius = 0.9f;

  // A 8x8 grid with a flower in each cell.
  for (unsigned y = 0u ; y < 8u ; ++y) {
    for (unsigned x = 0u ; x < 8u ; ++x) {
      sd.x = x;
      sd.y = y;

      sd.sprite.pack = /** index of the texture pack holding the flowers **/;
      sd.sprite.id = 0;
      sd.sprite.tint = olc::WHITE;
      sd.sprite.sprite = olc::vi2d(
        /** FIXME: determine the sprite index in the pack **/,
        /** FIXME: determine the sprite variation **/
      );

      drawSprite(sd, res.cf);
    }
  }
}
```

The code above performs the rendering of a 8x8 square of sprites taken from the pack loaded in the example above. The part to determine the index of the sprite based on the element to display is left to the user: it could come from fetching the particular element at this coordinate in the world or determined at random or anything else.

The `drawSprite` method expects the coordinates to be expressed in world coordinates and will automatically convert them in pixels coordinates based on the current position of the viewport in the world.

We also provide a `drawWarpedSprite` which ignores the radius to define the tile as exactly one tile big (given the current zoom level and coordinate frame).

### Scheduling

Whenever the app is running, the main loop is called. From the `App` perspective, this means that two methods are called in the following order:
* onInputs
* onFrame

#### onInputs

The user can easily add hot keys through the [Keys](src/lib/app/Controls.hh) enumeration: specific processes can then be triggered when the key is pressed. Note that the code in the [PGEApp::handleInputs](src/lib/app/PGEApp.cc) method should also be updated to detect the key being hit or released.

```cpp
void
App::onInputs(const controls::State& c,
              const CoordinateFrame& cf)
{
  /* ... */
  if (c.keys[controls::keys::P]) {
    m_game->togglePause();
  }
}
```

In general, it allows to react to some input and change the state of the game. It is in general not advised to perform graphic changes here but rather to configure everything so that the drawing routines can handle the visual representation.

Typically in a sudoku app the `onInputs` method would be responsible to fill in a digit in a specific cell but wouldn't actually render it, this would be done by the `drawXYZ` routines.

#### onFrame

This method handle the game logic. By default the code looks like this:
```cpp
bool
App::onFrame(float fElapsed) {
  // Handle case where no game is defined.
  if (m_game == nullptr) {
    return false;
  }

  if (!m_game->step(fElapsed)) {
    info("This is game over");
  }

  return m_game->terminated();
}
```

As we can see, we already handle the case where the game is terminated (meaning that we need to stop the application) and also call the `step` method in the [Game](#the-game-class) class. This can be extended with other processes which need to be called on each frame: a world simulation, some network connection handling, etc.

In general note that this method is synchronous with the rendering which means that any blocking/heavy operations done here will impact negatively the framerate. For a simple application it can be enough to just put processing here but if it becomes too complex it probably makes sense to resort to a more complex asynchronous system where the world has an execution thread separated from the actual execution of the rendering loop.

## The Game class

The [Game](src/lib/game/Game.hh) class provides a context to handle the execution of the code related to the game. The game is a bit of a blanket for the app specific logic. In the case of a sudoku app it can mean handling the verification to place a digit. In the case of a real game it can mean running the simulation and moving entities around.

The recommended approach is to create a specialized class (e.g. `World`) and use composition to make the `Game` class aware of it.

Another responsibility of the class is to handle the UI of the app: it should generate the various controls to interact with the app and also transmit the interaction of the user to apply them to the game (for example through the `World` class).

Several hooks are provided to customize the behavior easily.

### generateMenus

The `generateMenus` class is called by the `App` class and is supposed to create the menus to display when the user is on the `Game` screen. Any menu returned here will be automatically considered by the `App` framework and passed on events in case they are relevant.

The method is passed on the width and height of the rendering canvas which can help adapt the size of the menus to the actual size of the application.

Typically the user can create dedicated methods like so:

```cpp
std::vector<MenuShPtr>
Game::generateMenus(float /*width*/,
                    float /*height*/)
{
  auto statusBarMenus = generateStatusBarMenus();
  auto controlPanelMenus = generateControlPanelMenus();
  /* ... */

  std::vector<MenuShPtr> out;
  out.insert(statusBarMenus.begin(), statusBarMenus.end());
  out.insert(controlPanelMenus.begin(), controlPanelMenus.end());

  return out;
}
```

### How to store the menus

The `Game` class also defines an internal convenience structure to regroup all the menus that might be needed by the `App`:

```cpp
/// @brief - Convenience structure allowing to regroup
/// all info about the menu in a single struct.
struct Menus {
  /// FIXME: Add menus here.
};
```
The user can add necessary menus here to group them in a neat object to pass around.

**NOTE:** all menus do not need to be registered. It is mainly interesting to keep them in case their content needs to be updated (typically a label) during the game. Otherwise, the menus will automatically handle the release of their children sub-menus when being destroyed. The concept of actions (see the corresponding [section](#attach-an-action-to-a-menu)) should be sufficient to trigger processes on the `Game` (and also on elements of the `World` class for example) when the user clicks on a [Menu](https://github.com/Knoblauchpilze/pge-app/blob/master/src/lib/ui/Menu.hh).

### Attach an action to a menu

Each menu created with the [Menu](https://github.com/Knoblauchpilze/pge-app/blob/master/src/lib/ui/Menu.hh) class can be attached an [Action](https://github.com/Knoblauchpilze/pge-app/blob/master/src/lib/ui/Action.hh) which is triggered by the menu whenever it is clicked upon (provided that the menu is `clickable`).

Such an action is defined as follows:
```cpp
bool clickable = true;
pge::MenuShPtr m = std::make_shared<pge::Menu>(/* arguments */, clickable, /* arguments */);
m->setSimpleAction(
  [/* optional capture of variables */](Game& g) {
    /// FIXME: Call any method of the game.
  }
);
```
The action receives a reference to the game and can trigger an action on it. A typical use case is to create a public method in the `Game` class (say `Game::foo`) and then have it called by the simple action. This could be for example the creation of a new element, the update of a certain attribute, etc.

Actions can be reset and can also include variables from the context creating it, such as a certain value or a compile time parameter. This mechanism proved quite reliable and easy-to-use to trigger processes on the `Game` from the UI.

### Timed menu

The `Game` class defines a convenience internal structure called a `TimedMenu` which allows to display a menu for a certain period of time based on a certain condition.

The user needs to configure the menu (usually in the [generateMenus](#generatemenus) method) and then update its status in the `updateUI` function like so:

```cpp
std::vector<MenuShPtr>
Game::generateMenus(float width, float height) {
  m_menus.timed.date = utils::TimeStamp();
  m_menus.timed.wasActive = false;
  /// NOTE: The duration is expressed in milliseconds.
  m_menus.timed.duration = 3000;
  m_menus.timed.menu = /** FIXME: Generate menu **/
  /// NOTE: Disable the visibility at first if needed.
  m_menus.timed.menu->setVisible(false);

  /* ... */
}

void
Game::updateUI() {
  /* ... */

  m_menus.timed.update(/* Your condition */);

  /* ... */
}
```

This allows to easily display alerts for example, or any information that needs to be presented to the user for a short period of time. Note that it is usually recommended to set the `clickable` and `selectable` flags of the menu attached to the component.

The condition to provide in the update should be `true` when the menu is supposed to be visible and `false` otherwise. The base class already provides the mechanism so that the menu can be reactivated as needed without any additional processing.

### The step method

The main method is probably `step` where the user can perform the update of the elements that will be added to the `Game` class. Once again, this plays nicely with the concept of a `World` class which would be responsible for the simulation.

The user is given the elapsed time since the last frame in seconds. This information is sufficient to handle menus for example but in general it shouldn't be considered as the main way to measure time passing in the simulation.

```cpp
bool
Game::step(float /*tDelta*/) {
  // When the game is paused it is not over yet.
  if (m_state.paused) {
    return true;
  }

  info("Perform step method of the game");

  updateUI();

  return true;
}
```

One pre-filled process happening in the `step` method is the `updateUI` method: this is responsible to make sure that menus also get the chance to be updated each frame. More info in the dedicated [section](#updateUI-or-how-menus-live)

### updateUI or how menus live

The `updateUI` method is very similar to the `step` method but is dedicated to the UI. It allows the menus to also be aware of the passage of time in the simulation and to update their content with meaningful information.

Such an update is important for two kinds of menus:
* timed menus
* menus which information changes over time

The static menus don't need this. A non-static menu is for example a label indicating how much gold a player has in a game: this typically changes based on the events of the game and should be updated regularly.

### Make changes to the game

The last hook provided by the `Game` class is the `performAction` method. It looks like so:
```cpp
void
Game::performAction(float /*x*/, float /*y*/) {
  // Only handle actions when the game is not disabled.
  if (m_state.disabled) {
    log("Ignoring action while menu is disabled");
    return;
  }
}
```

Whenever the user clicks on the game and doesn't target directly a menu, the `Game::performAction` method is called. It is passed on the coordinates of the click in tiles using a floating point format (meaning that the `Game` can access the intra-tile position).

By default actions are ignored if the game is disabled: this corresponds to the `Game` being paused. The user is free to add any code to create in-game element whenever the user clicks somewhere. For example, display a label if the user clicks on a building somewhere in the game, or spawn a new entity.

## The GameState class

The [GameState](src/lib/game/GameState.hh) class provides the high level state management for the screens of the application. With _Screen_ we mean for example the welcome screen, the load game screen, the game over screen or the main game screen.

In order to transition from one screen to the other and making sure that when a phase ends we can go to the next logical one is handled by this class.

### An application as a state machine

The application can be seen as a state machine which can transition to various screens based on the actions of the user.

![State machine](resources/screens_state_machine.png)

As we can see, the _Home_ screen is a root and allows to transition to the _Game_ or _Load_ game screen. The `GameState` manages the transitions and hide this behind an enumeration called [Screen](src/lib/game/GameState.hh):
```cpp
enum class Screen
{
  Home,
  LoadGame,
  Game,
  GameOver,
  Exit
};
```

The app is then using the current state in the `drawDecal` (and other related routines) like so:
```cpp
void App::drawDecal(const RenderDesc &res)
{
  /* ... */

  if (m_state->getScreen() != Screen::Game)
  {
    return;
  }

  /* ... */
}
```

So we only consider rendering something when the screen is set to `Game`. On the other hand the state is rendered in the `draw` method like so:
```cpp
void App::draw(const RenderDesc & /*res*/)
{
  /* ... */

  if (m_state->getScreen() != Screen::Game)
  {
    m_state->render(this);
    return;
  }

  /* ... */
}
```

This guarantees that the menus are rendered as long as we're not in the `Game` screen. As soon as we leave it again then the menus are also rendered. The default application defines a simple state machine with a few screens, but the user can configure new ones if needed.

### Adding new screens

It might be useful to create new screens: for example in a chess app, we might want to add some screens corresponding to selecting the difficulty of the AI or which color to play. To do so, the first step is to add a value in the enumeration and then generate the corresponding menu. The existing menus are generated in the constructor of the `GameState`:
```cpp
GameState::GameState(const olc::vi2d& dims,
                     const Screen& screen):
  utils::CoreObject("state"),

  /* ... */

  m_home(nullptr),
  m_loadGame(nullptr),
  m_gameOver(nullptr)
  /** NOTE: Add an attribute for the menu **/
{
  setService("chess");

  generateHomeScreen(dims);
  generateLoadGameScreen(dims);
  generateGameOverScreen(dims);
  /** NOTE: Add a generation method **/

  /* ... */
}
```

When generating the new menu, one can add transition like so:
```cpp
m->setSimpleAction([this](Game & /*g*/) {
  /* ... */
  setScreen(Screen::YourNewScreen);
});
```

The rest of the application should behave correctly as long as the transitions are properly wired.

### Default journey in the app

Most applications start on the home screen:
![Home screen](resources/home_screen.png)

From there the user can select a new game:
![Game screen](resources/main_view.png)

This view is empty in the default application and display the debug layer. The user can freely refine it by adding menus and displaying the actual content of the game.

Finally the user can select to load an existing 'game' (whatever it can mean):
![Load game](resources/load_game.png)

### Saved games

We provide a convenience structure to handle the saved games. By default the `GameState` has an attribute called `m_savedGames` which allows to perform the generation of a menu to present the saved games to the user.

The configuration includes how many games should be displayed in a single page, the directory where saved games should be fetched and the extension of the files defining saved games:
```cpp
m_savedGames(10u, "data/saves", "ext")
```

Once the user picks a saved game, a signal is emitted by this object and transmitted to the `GameState` class (the connection is already active). To react to such an event, the user has to elaborate the implementation of the dedicated method:
```cpp
void
GameState::onSavedGamePicked(const std::string& game) {
  info("Picked saved game \"" + game + "\"");
  setScreen(Screen::Game);
}
```

For now nothing is done and the application switches to the `Game` state immediately. A popular way to deal with the loading of a game is to add an internal attribute to the `GameState` class which represents the `Game` (typically a reference) and then call a newly created method such as `loadNewGame(const std::string& file)`. The input attribute `game` is the full path to the saved file data.

The user should also modify the `App::loadMenuResources` method to account for the modified signature of the `GameState` constructor.

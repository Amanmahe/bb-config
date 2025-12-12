#include <filesystem>
#include <fstream>
#include <regex>
#include "ftxui/component/component.hpp"
#include "ftxui/dom/elements.hpp"
#include "ui/panel/panel.hpp"

using namespace ftxui;

namespace ui {

const std::vector<std::string> iOentries = {
    "IN",
    "OUT",
};

const std::vector<std::string> valueEntries = {
    "0",
    "1",
};

const std::vector<std::string> activeEntries = {
    "Low",
    "High",
};

const std::vector<std::string> edgeEntries = {
    "Pos (+ve)",
    "Neg (-ve)",
    "Any",
    "None",
};

class Gpio : public ComponentBase {
 public:
  Gpio(std::string path, int* tab, int* next, int* limit) : path_(path) {
    limit_ = limit;
    tab_ = tab;
    next_ = next;
    Fetch();
    BuildUI();
  }

  std::string label() const { return label_; }
  std::string direction() const { return direction_; }
  std::string edge() const { return edge_; }
  std::string value() const { return value_; }
  std::string active_low() const { return active_low_; }

 private:
  void Fetch() {
    std::ifstream label_file(path_ + "/label");
    if (label_file.is_open()) {
      std::getline(label_file, label_);
    } else {
      label_ = "Unknown";
    }
    
    std::ifstream edge_file(path_ + "/edge");
    if (edge_file.is_open()) {
      std::getline(edge_file, edge_);
    } else {
      edge_ = "none";
    }
    
    std::ifstream direction_file(path_ + "/direction");
    if (direction_file.is_open()) {
      std::getline(direction_file, direction_);
    } else {
      direction_ = "in";
    }
    
    std::ifstream value_file(path_ + "/value");
    if (value_file.is_open()) {
      std::getline(value_file, value_);
    } else {
      value_ = "0";
    }
    
    std::ifstream active_low_file(path_ + "/active_low");
    if (active_low_file.is_open()) {
      std::getline(active_low_file, active_low_);
    } else {
      active_low_ = "0";
    }
  }

  void StoreDirection(std::string direction) {
    std::ofstream(path_ + "/direction") << direction;
    Fetch();
  };

  void StoreEdge(std::string edge) {
    std::ofstream(path_ + "/edge") << edge;
    Fetch();
  };

  void StoreValue(std::string value) {
    std::ofstream(path_ + "/value") << value;
    Fetch();
  };

  void StoreActiveLow(std::string active_low) {
    std::ofstream(path_ + "/active_low") << active_low;
    Fetch();
  };

  void handleDirection() {
    if (ioTog == 0)
      StoreDirection("in");
    else
      StoreDirection("out");
  }

  void handleValue() {
    if (valTog == 0)
      StoreValue("0");
    else
      StoreValue("1");
  }

  void handleActiveLow() {
    if (activeTog == 0)
      StoreActiveLow("0");
    else
      StoreActiveLow("1");
  }

  void handleEdge() {
    switch (edgeTog) {
      case 0:
        StoreEdge("rising");
        break;
      case 1:
        StoreEdge("falling");
        break;
      case 2:
        StoreEdge("both");
        break;
      case 3:
        StoreEdge("none");
        break;
    }
  }

  void BuildUI() {
    auto iOtoggleOpt = MenuOption::Toggle();
    auto valueToggleOpt = MenuOption::Toggle();
    auto activeToggleOpt = MenuOption::Toggle();
    auto edgeToggleOpt = MenuOption::Toggle();
    iOtoggleOpt.on_enter = [&] { handleDirection(); };
    valueToggleOpt.on_enter = [&] { handleValue(); };
    activeToggleOpt.on_enter = [&] { handleActiveLow(); };
    edgeToggleOpt.on_enter = [&] { handleEdge(); };
    ioToggle = Menu(&iOentries, &ioTog, iOtoggleOpt);
    valToggle = Menu(&valueEntries, &valTog, valueToggleOpt);
    activeToggle = Menu(&activeEntries, &activeTog, activeToggleOpt);
    edgeToggle = Menu(&edgeEntries, &edgeTog, edgeToggleOpt);
    
    // Set toggle states based on current values
    ioTog = (direction_ == "out") ? 1 : 0;
    valTog = (value_ == "1") ? 1 : 0;
    activeTog = (active_low_ == "1") ? 1 : 0;
    
    if (edge_ == "rising") edgeTog = 0;
    else if (edge_ == "falling") edgeTog = 1;
    else if (edge_ == "both") edgeTog = 2;
    else edgeTog = 3;
    
    Component actions = Renderer(
        Container::Vertical({
            ioToggle,
            valToggle,
            activeToggle,
            edgeToggle,
        }),
        [&] {
          return vbox({
              text(label_ + " Status "),
              hbox(text(" * Direction       : "), text(direction_)),
              hbox(text(" * Value           : "), text(value_)),
              hbox(text(" * Active Low      : "), text(active_low_)),
              hbox(text(" * Edge            : "), text(edge_)),
              text(" Actions "),
              hbox(text(" * Direction       : "), ioToggle->Render()),
              hbox(text(" * Value           : "), valToggle->Render()),
              hbox(text(" * Active Low      : "), activeToggle->Render()),
              hbox(text(" * Edge            : "), edgeToggle->Render()),
          });
        });
    Add(Container::Vertical(
        {actions, Container::Horizontal({
                      Button("Back to Menu", [&] { *tab_ = 0; }),
                      Button("Prev",
                             [&] {
                               (*next_)--;
                               if (*next_ < 0) {
                                 *next_ = (*limit_ > 0) ? *limit_ - 1 : 0;
                               }
                             }),
                      Button("Next",
                             [&] {
                               (*next_)++;
                               if (*next_ >= *limit_) {
                                 *next_ = 0;
                               }
                             }),
                  })}));
  }

  std::string path_;
  std::string label_;
  std::string direction_;
  std::string edge_;
  std::string value_;
  std::string active_low_;
  int* tab_;
  int* next_;
  int* limit_;
  int ioTog = 0;
  int activeTog = 0;
  int valTog = 0;
  int edgeTog = 3;
  Component ioToggle;
  Component activeToggle;
  Component valToggle;
  Component edgeToggle;
};

class GPIOImpl : public PanelBase {
 public:
  GPIOImpl() { BuildUI(); }
  std::string Title() override { return "GPIO"; }

 private:
  // Helper function to check if a GPIO is a P1/P2 pin
  bool isBeagleBonePin(const std::string& label) {
    // Check for P1 or P2 pins using regex
    std::regex pin_pattern(R"(P[12]\.\d+)");
    if (std::regex_search(label, pin_pattern)) {
      return true;
    }
    
    // Also check for pins that have (P1.x) or (P2.x) in parentheses
    std::regex paren_pattern(R"(\(P[12]\.\d+[^)]*\))");
    if (std::regex_search(label, paren_pattern)) {
      return true;
    }
    
    return false;
  }

  void BuildUI() {
    MenuOption menuOpt;
    menuOpt.on_enter = [&] { tab = 1; };
    gpio_menu = Menu(&gpio_names, &selected, menuOpt);
    gpio_individual = Container::Vertical({}, &selected);
    
    // First, let's look at all GPIO directories
    for (const auto& it : std::filesystem::directory_iterator("/sys/class/gpio/")) {
      std::string gpio_path = it.path();
      
      // Skip gpiochip directories
      if (gpio_path.find("gpiochip") != std::string::npos) {
        continue;
      }
      
      // Check if it's a GPIO directory (starts with "gpio")
      if (gpio_path.find("/sys/class/gpio/gpio") == 0) {
        // Read the label
        std::string label_path = gpio_path + "/label";
        std::ifstream label_file(label_path);
        std::string label;
        
        if (label_file.is_open()) {
          std::getline(label_file, label);
        } else {
          // If no label file, use the GPIO number
          label = it.path().filename().string();
        }
        
        // Check if this is a P1/P2 pin
        if (isBeagleBonePin(label)) {
          auto gpio = std::make_shared<Gpio>(gpio_path, &tab, &selected, &limit);
          children_.push_back(gpio);
          gpio_individual->Add(gpio);
          limit++;
        }
      }
    }

    Add(Container::Tab(
        {
            gpio_menu,
            gpio_individual,
        },
        &tab));
  }

  Element Render() override {
    gpio_names.clear();
    for (const auto& child : children_) {
      gpio_names.push_back(child->label());
    }

    if (tab == 1) {
      if (children_.empty()) {
        return window(text("GPIO Control"),
                      text("No GPIO pins found. Try exporting GPIOs first.") | 
                      center | flex);
      }
      
      if (selected >= 0 && selected < static_cast<int>(children_.size())) {
        return children_[selected]->Render();
      } else {
        selected = 0;
        if (!children_.empty()) {
          return children_[0]->Render();
        }
      }
    }

    return window(text("GPIO Menu"),
                  gpio_menu->Render() | vscroll_indicator | frame | flex);
  }

  std::vector<std::shared_ptr<Gpio>> children_;
  std::vector<std::string> gpio_names;
  Component gpio_menu;
  Component gpio_individual;
  int selected = 0;
  int tab = 0;
  int limit = 0;
};

namespace panel {
Panel GPIO() {
  return Make<GPIOImpl>();
}
}  // namespace panel
}  // namespace ui
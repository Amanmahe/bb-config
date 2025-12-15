#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <vector>
#include <map>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <array>
#include <unistd.h>  // for usleep
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
  Gpio(std::string gpio_num, std::string label, int* tab, int* next, int* limit) 
      : gpio_num_(gpio_num), label_(label) {
    limit_ = limit;
    tab_ = tab;
    next_ = next;
    path_ = "/sys/class/gpio/gpio" + gpio_num;
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
    // Read direction
    std::ifstream direction_file(path_ + "/direction");
    if (direction_file.is_open()) {
      std::getline(direction_file, direction_);
    } else {
      direction_ = "in";
    }
    
    // Read edge
    std::ifstream edge_file(path_ + "/edge");
    if (edge_file.is_open()) {
      std::getline(edge_file, edge_);
    } else {
      edge_ = "none";
    }
    
    // Read value
    std::ifstream value_file(path_ + "/value");
    if (value_file.is_open()) {
      std::getline(value_file, value_);
    } else {
      value_ = "0";
    }
    
    // Read active_low
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
              text(label_ + " (GPIO " + gpio_num_ + ") Status "),
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
  std::string gpio_num_;
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

// Helper function to execute shell command and get output
std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, int(*)(FILE*)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        return "";
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

class GPIOImpl : public PanelBase {
 public:
  GPIOImpl() { BuildUI(); }
  std::string Title() override { return "GPIO"; }

 private:
  // Get GPIO pin information using gpioinfo
  std::vector<std::pair<int, std::string>> getGpioPins() {
    std::vector<std::pair<int, std::string>> pins;
    
    std::string output = exec("gpioinfo 2>/dev/null");
    if (output.empty()) {
      return pins;
    }
    
    std::istringstream iss(output);
    std::string line;
    
    // Chip number to GPIO base mapping (from your earlier output)
    std::map<int, int> chip_to_base = {
      {0, 512},  // gpiochip0 -> base 512
      {1, 515},  // gpiochip1 -> base 515
      {2, 539},  // gpiochip2 -> base 539
      {3, 631},  // gpiochip3 -> base 631
      {4, 683},  // gpiochip4 -> base 683
    };
    
    int current_chip = -1;
    
    while (std::getline(iss, line)) {
      // Check for chip header
      if (line.find("gpiochip") != std::string::npos) {
        // Extract chip number
        size_t chip_pos = line.find("gpiochip");
        if (chip_pos != std::string::npos) {
          std::string chip_num_str;
          for (size_t i = chip_pos + 8; i < line.size() && isdigit(line[i]); i++) {
            chip_num_str += line[i];
          }
          if (!chip_num_str.empty()) {
            current_chip = std::stoi(chip_num_str);
          }
        }
        continue;
      }
      
      // Check for pin line
      if (line.find("line") != std::string::npos && current_chip >= 0 && 
          chip_to_base.find(current_chip) != chip_to_base.end()) {
        
        // Extract line number
        size_t line_pos = line.find("line");
        if (line_pos != std::string::npos) {
          // Find the colon after line number
          size_t colon_pos = line.find(":", line_pos);
          if (colon_pos != std::string::npos) {
            // Get line number string (between "line" and ":")
            std::string line_num_str = line.substr(line_pos + 4, colon_pos - line_pos - 4);
            
            // Clean up the string
            line_num_str.erase(0, line_num_str.find_first_not_of(" \t"));
            line_num_str.erase(line_num_str.find_last_not_of(" \t") + 1);
            
            if (!line_num_str.empty()) {
              int line_num = std::stoi(line_num_str);
              
              // Extract label - look for text in quotes
              std::string label;
              size_t quote_start = line.find('\"');
              if (quote_start != std::string::npos) {
                size_t quote_end = line.find('\"', quote_start + 1);
                if (quote_end != std::string::npos) {
                  label = line.substr(quote_start + 1, quote_end - quote_start - 1);
                }
              }
              
              // Check if it's a P1/P2 pin or USR_LED
              if (!label.empty() && 
                  (label.find("P1.") != std::string::npos || 
                   label.find("P2.") != std::string::npos ||
                   label.find("USR_LED") != std::string::npos)) {
                
                // Calculate GPIO number
                int base = chip_to_base[current_chip];
                int gpio_num = base + line_num;
                
                pins.push_back({gpio_num, label});
              }
            }
          }
        }
      }
    }
    
    return pins;
  }

  // Export a GPIO if not already exported
  bool exportGpio(int gpio_num) {
    std::string gpio_path = "/sys/class/gpio/gpio" + std::to_string(gpio_num);
    
    // Check if already exported
    if (std::filesystem::exists(gpio_path)) {
      return true;
    }
    
    // Try to export
    std::ofstream export_file("/sys/class/gpio/export");
    if (export_file.is_open()) {
      export_file << gpio_num;
      export_file.close();
      
      // Wait for export to complete
      usleep(50000); // 50ms
      
      return std::filesystem::exists(gpio_path);
    }
    
    return false;
  }

  void BuildUI() {
    // Get list of GPIO pins from gpioinfo
    auto gpio_pins = getGpioPins();
    
    MenuOption menuOpt;
    menuOpt.on_enter = [&] { tab = 1; };
    gpio_menu = Menu(&gpio_names, &selected, menuOpt);
    gpio_individual = Container::Vertical({}, &selected);
    
    if (!gpio_pins.empty()) {
      // Process pins from gpioinfo
      for (const auto& pin : gpio_pins) {
        int gpio_num = pin.first;
        std::string label = pin.second;
        
        // Try to export the GPIO
        if (exportGpio(gpio_num)) {
          std::string gpio_num_str = std::to_string(gpio_num);
          auto gpio = std::make_shared<Gpio>(gpio_num_str, label, &tab, &selected, &limit);
          children_.push_back(gpio);
          gpio_individual->Add(gpio);
          limit++;
        }
      }
    }
    
    // If no pins found, show error message
    if (children_.empty()) {
      error_message_ = "No GPIO pins found. Make sure:\n"
                       "1. gpiod is installed (sudo apt-get install gpiod)\n"
                       "2. Run with sudo privileges\n"
                       "3. GPIOs are accessible";
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
                      vbox({
                          text("No GPIO pins found."),
                          text(""),
                          text("Possible issues:"),
                          text("1. gpiod not installed - run: sudo apt-get install gpiod"),
                          text("2. Need sudo privileges - run program with sudo"),
                          text("3. GPIOs not exported - run export script"),
                          text(""),
                          text("Try running:"),
                          text("  sudo ./export_beaglebone_pins.sh"),
                          text("  sudo gpioinfo")
                      }) | center | flex);
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

    // Display GPIO menu
    if (children_.empty()) {
      return window(text("GPIO Menu"),
                    vbox({
                        text("Available GPIOs: 0"),
                        text(""),
                        text("No GPIO pins detected."),
                        text("Try running with sudo or check installation.")
                    }) | center | flex);
    }

    return window(text("GPIO Menu - " + std::to_string(children_.size()) + " pins"),
                  gpio_menu->Render() | vscroll_indicator | frame | flex);
  }

  std::vector<std::shared_ptr<Gpio>> children_;
  std::vector<std::string> gpio_names;
  Component gpio_menu;
  Component gpio_individual;
  std::string error_message_;
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
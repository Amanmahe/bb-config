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
  // Dynamic GPIO chip detection for any board
  std::vector<std::pair<int, std::string>> getGpioPins() {
    std::vector<std::pair<int, std::string>> detected_pins;  
    
    // First, try to get chip info using gpioinfo
    std::string output = exec("gpioinfo 2>/dev/null");
    
    // If gpioinfo fails, try alternative methods
    if (output.empty()) {
      // Try to detect GPIOs from /sys/class/gpio
      return detectGpioFromSysfs();
    }
    
    std::istringstream iss(output);
    std::string line;
    std::map<int, int> chip_to_base;
    int current_chip = -1;
    
    // First pass: collect chip information
    while (std::getline(iss, line)) {
      // Look for chip information line
      if (line.find("gpiochip") != std::string::npos && 
          line.find("lines") != std::string::npos) {
        
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
        
        // Extract base number from the line
        std::regex base_regex("\\[([0-9]+)\\s*-");
        std::smatch base_match;
        if (std::regex_search(line, base_match, base_regex) && base_match.size() > 1) {
          int base = std::stoi(base_match[1]);
          chip_to_base[current_chip] = base;
        }
        continue;
      }
    }
    
    // Second pass: collect GPIO pins
    iss.clear();
    iss.str(output);
    current_chip = -1;
    
    while (std::getline(iss, line)) {
      // Check for chip header
      if (line.find("gpiochip") != std::string::npos && 
          line.find("lines") != std::string::npos) {
        // Extract chip number again
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
      
      // Check for pin line with a label
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
              
              // Check for common GPIO labels
              bool is_user_gpio = false;
              
              if (!label.empty()) {
                // Check for common GPIO patterns
                if (label.find("GPIO") != std::string::npos ||
                    label.find("P") != std::string::npos ||
                    label.find("USR") != std::string::npos ||
                    label.find("LED") != std::string::npos ||
                    label.find("BTN") != std::string::npos) {
                  is_user_gpio = true;
                }
              } else {
                // If no label, check if it's a user-accessible GPIO by checking the name field
                size_t name_pos = line.find("name:", colon_pos);
                if (name_pos != std::string::npos) {
                  size_t name_end = line.find(",", name_pos);
                  if (name_end != std::string::npos) {
                    label = line.substr(name_pos + 5, name_end - name_pos - 5);
                    label.erase(0, label.find_first_not_of(" \t"));
                    label.erase(label.find_last_not_of(" \t") + 1);
                    
                    if (label.find("GPIO") != std::string::npos ||
                        label.find("P") != std::string::npos) {
                      is_user_gpio = true;
                    }
                  }
                }
              }
              
              if (is_user_gpio) {
                // Calculate GPIO number
                int base = chip_to_base[current_chip];
                int gpio_num = base + line_num;
                
                // Use label or create generic one
                if (label.empty()) {
                  label = "GPIO_" + std::to_string(gpio_num);
                }
                
                detected_pins.push_back({gpio_num, label});  
              }
            }
          }
        }
      }
    }
    
    return detected_pins;  
  }

  // Fallback GPIO detection from /sys/class/gpio
  std::vector<std::pair<int, std::string>> detectGpioFromSysfs() {
    std::vector<std::pair<int, std::string>> sysfs_pins;  
    
    // Check if /sys/class/gpio exists
    if (!std::filesystem::exists("/sys/class/gpio")) {
      return sysfs_pins;  
    }
    
    // Look for already exported GPIOs
    for (const auto& entry : std::filesystem::directory_iterator("/sys/class/gpio")) {
      std::string entry_name = entry.path().filename().string();
      
      if (entry_name.find("gpio") == 0 && entry_name != "gpiochip") {
        // Extract GPIO number
        std::string gpio_num_str = entry_name.substr(4);
        
        try {
          int gpio_num = std::stoi(gpio_num_str);
          
          // Check if it's a valid GPIO (not a chip)
          std::string label_path = entry.path().string() + "/label";
          std::string label = "GPIO_" + gpio_num_str;
          
          if (std::filesystem::exists(label_path)) {
            std::ifstream label_file(label_path);
            if (label_file.is_open()) {
              std::getline(label_file, label);
              if (label.empty()) {
                label = "GPIO_" + gpio_num_str;
              }
            }
          }
          
          sysfs_pins.push_back({gpio_num, label});  
        } catch (...) {
          // Skip if not a number
          continue;
        }
      }
    }
    
    return sysfs_pins; 
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

  // Try to export common GPIOs for testing
  void tryExportCommonGpios() {
    // Try exporting some common GPIO numbers for testing
    std::vector<int> common_gpios = {2, 3, 4, 17, 27, 22, 10, 9, 11, 0, 5, 6, 13, 19, 26, 14, 15, 18, 23, 24, 25, 8, 7, 1, 12, 16, 20, 21};
    
    for (int gpio : common_gpios) {
      exportGpio(gpio);
      usleep(10000); // 10ms between exports
    }
  }

  void BuildUI() {
    // First, try exporting some common GPIOs for testing
    tryExportCommonGpios();
    
    // Get list of GPIO pins
    auto gpio_pins = getGpioPins();
    
    // If no pins found, show all available GPIOs from 0-1023
    if (gpio_pins.empty()) {
      for (int i = 0; i < 1024; i++) {
        if (exportGpio(i)) {
          std::string label = "GPIO_" + std::to_string(i);
          pins.push_back({i, label});
        }
      }
    } else {
      pins = gpio_pins;
    }
    
    MenuOption menuOpt;
    menuOpt.on_enter = [&] { tab = 1; };
    gpio_menu = Menu(&gpio_names, &selected, menuOpt);
    gpio_individual = Container::Vertical({}, &selected);
    
    if (!pins.empty()) {
      // Process detected pins
      for (const auto& pin : pins) {
        int gpio_num = pin.first;
        std::string label = pin.second;
        
        // Try to export the GPIO (if not already)
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
                       "1. Run with sudo privileges\n"
                       "2. GPIOs are accessible\n"
                       "3. Check board compatibility";
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
                          text("1. Need sudo privileges - run program with sudo"),
                          text("2. Board not detected - trying common GPIOs"),
                          text(""),
                          text("Trying to export common GPIOs..."),
                          separator(),
                          text("Manually export GPIOs:"),
                          text("  echo <number> > /sys/class/gpio/export"),
                          text(""),
                          text("Example: sudo echo 17 > /sys/class/gpio/export")
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
                        text("Try running with sudo or check board compatibility.")
                    }) | center | flex);
    }

    return window(text("GPIO Menu - " + std::to_string(children_.size()) + " pins detected"),
                  gpio_menu->Render() | vscroll_indicator | frame | flex);
  }

  std::vector<std::pair<int, std::string>> pins;
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
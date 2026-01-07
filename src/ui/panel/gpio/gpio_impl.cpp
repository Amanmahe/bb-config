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
#include <iostream>  // for debug
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
  // Get GPIO pin information using gpioinfo - ONLY P pins
  std::vector<std::pair<int, std::string>> getGpioPins() {
    std::vector<std::pair<int, std::string>> pins;
    
    // First try with sudo
    std::string output = exec("sudo gpioinfo 2>/dev/null");
    
    // If empty, try without sudo
    if (output.empty()) {
      output = exec("gpioinfo 2>/dev/null");
    }
    
    if (output.empty()) {
      std::cerr << "ERROR: gpioinfo command failed or returned empty output\n";
      return pins;
    }
    
    std::istringstream iss(output);
    std::string line;
    
    // Map to store chip number to base GPIO number
    std::map<int, int> chip_to_base;
    
    // First pass: Collect chip base numbers from /sys
    for (int chip_num = 0; chip_num < 10; chip_num++) {
        std::string base_path = "/sys/class/gpio/gpiochip" + std::to_string(chip_num) + "/base";
        std::ifstream base_file(base_path);
        if (base_file.is_open()) {
            std::string base_str;
            if (std::getline(base_file, base_str)) {
                try {
                    int base = std::stoi(base_str);
                    chip_to_base[chip_num] = base;
                    std::cerr << "DEBUG: Chip " << chip_num << " base = " << base << "\n";
                } catch (...) {
                    // Ignore conversion errors
                }
            }
        }
    }
    
    int current_chip = -1;
    bool reading_chip = false;
    
    // Second pass: Parse gpioinfo output
    while (std::getline(iss, line)) {
        // Trim leading/trailing whitespace
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);
        
        // Check for chip header
        if (line.find("gpiochip") == 0) {
            // Extract chip number from "gpiochipX"
            std::string chip_str = line.substr(8); // Skip "gpiochip"
            size_t space_pos = chip_str.find_first_of(" :");
            if (space_pos != std::string::npos) {
                chip_str = chip_str.substr(0, space_pos);
            }
            
            try {
                current_chip = std::stoi(chip_str);
                std::cerr << "DEBUG: Found chip " << current_chip << "\n";
                reading_chip = true;
            } catch (...) {
                current_chip = -1;
                reading_chip = false;
            }
            continue;
        }
        
        // Skip if we're not reading a chip or line doesn't contain "line"
        if (!reading_chip || line.find("line") == std::string::npos) {
            continue;
        }
        
        // Parse line: "line   X:"
        size_t line_pos = line.find("line");
        if (line_pos == std::string::npos) continue;
        
        // Extract line number
        size_t num_start = line_pos + 4;
        while (num_start < line.size() && (line[num_start] == ' ' || line[num_start] == '\t')) {
            num_start++;
        }
        
        size_t num_end = num_start;
        while (num_end < line.size() && isdigit(line[num_end])) {
            num_end++;
        }
        
        if (num_end == num_start) continue;
        
        try {
            int line_num = std::stoi(line.substr(num_start, num_end - num_start));
            
            // Now look for P pin in the line
            // First check in quotes
            std::string pin_name = "";
            size_t quote_start = line.find('\"');
            if (quote_start != std::string::npos) {
                size_t quote_end = line.find('\"', quote_start + 1);
                if (quote_end != std::string::npos) {
                    std::string quoted = line.substr(quote_start + 1, quote_end - quote_start - 1);
                    
                    // Check if quoted text starts with P followed by a digit
                    if (quoted.size() >= 2 && 
                        (quoted[0] == 'P' || quoted[0] == 'p') && 
                        isdigit(quoted[1])) {
                        pin_name = quoted;
                    }
                }
            }
            
            // If not found in quotes, search entire line for P followed by digit
            if (pin_name.empty()) {
                // Search for P or p followed by digit
                for (size_t i = 0; i < line.size(); i++) {
                    if ((line[i] == 'P' || line[i] == 'p') && 
                        i + 1 < line.size() && isdigit(line[i + 1])) {
                        
                        // Extract the pin name
                        size_t start = i;
                        size_t end = i + 1;
                        while (end < line.size() && 
                               (isdigit(line[end]) || line[end] == '.' || 
                                line[end] == '_' || line[end] == '(')) {
                            end++;
                        }
                        
                        pin_name = line.substr(start, end - start);
                        
                        // Clean up trailing special characters
                        while (!pin_name.empty() && 
                               (pin_name.back() == '(' || pin_name.back() == ' ' || 
                                pin_name.back() == '\t')) {
                            pin_name.pop_back();
                        }
                        
                        break;
                    }
                }
            }
            
            // If we found a P pin, add it
            if (!pin_name.empty()) {
                // Calculate GPIO number
                int gpio_num = -1;
                
                if (chip_to_base.find(current_chip) != chip_to_base.end()) {
                    gpio_num = chip_to_base[current_chip] + line_num;
                } else {
                    // Estimate: assume chips are numbered sequentially with 32 lines each
                    // This is a common pattern for many boards
                    gpio_num = current_chip * 32 + line_num;
                }
                
                std::cerr << "DEBUG: Found P pin - Chip " << current_chip 
                          << ", Line " << line_num << ", GPIO " << gpio_num 
                          << ", Name: " << pin_name << "\n";
                
                pins.push_back({gpio_num, pin_name});
            }
            
        } catch (...) {
            // Skip lines with conversion errors
            continue;
        }
    }
    
    // Sort by GPIO number
    std::sort(pins.begin(), pins.end(), 
              [](const auto& a, const auto& b) { return a.first < b.first; });
    
    // Remove duplicates
    pins.erase(std::unique(pins.begin(), pins.end(),
               [](const auto& a, const auto& b) { return a.first == b.first; }),
               pins.end());
    
    std::cerr << "DEBUG: Total P pins found: " << pins.size() << "\n";
    for (const auto& pin : pins) {
        std::cerr << "  GPIO " << pin.first << ": " << pin.second << "\n";
    }
    
    return pins;
  }

  // Alternative method if gpioinfo doesn't work
  std::vector<std::pair<int, std::string>> getGpioPinsFallback() {
    std::vector<std::pair<int, std::string>> pins;
    
    // Look for already exported GPIOs
    std::string cmd = "ls -d /sys/class/gpio/gpio[0-9]* 2>/dev/null";
    std::string result = exec(cmd.c_str());
    
    if (!result.empty()) {
        std::istringstream iss(result);
        std::string gpio_path;
        while (std::getline(iss, gpio_path)) {
            // Extract GPIO number
            size_t gpio_pos = gpio_path.find("gpio");
            if (gpio_pos != std::string::npos) {
                std::string num_str;
                for (size_t i = gpio_pos + 4; i < gpio_path.size() && isdigit(gpio_path[i]); i++) {
                    num_str += gpio_path[i];
                }
                
                if (!num_str.empty()) {
                    try {
                        int gpio_num = std::stoi(num_str);
                        pins.push_back({gpio_num, "GPIO" + num_str});
                    } catch (...) {
                        // Skip
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
    
    // If no P pins found, try fallback
    if (gpio_pins.empty()) {
        std::cerr << "WARNING: No P pins found with gpioinfo, trying fallback\n";
        gpio_pins = getGpioPinsFallback();
    }
    
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
        } else {
          std::cerr << "WARNING: Failed to export GPIO " << gpio_num << " (" << label << ")\n";
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
                          text("3. GPIOs not exported"),
                          text(""),
                          text("Debug info:"),
                          text("Tried to find P pins from gpioinfo"),
                          text("Check terminal for debug output")
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
                        text("No P pins detected."),
                        text("Looking for pins starting with 'P' (e.g., P8.10, P9.11)"),
                        text(""),
                        text("Check if:"),
                        text("1. Board has P pins"),
                        text("2. gpioinfo shows P pins"),
                        text("3. Run with: sudo gpioinfo | grep P")
                    }) | center | flex);
    }

    return window(text("GPIO Menu - " + std::to_string(children_.size()) + " P pins"),
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
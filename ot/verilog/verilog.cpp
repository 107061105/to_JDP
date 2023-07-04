#include <ot/verilog/verilog.hpp>

namespace ot::vlog {

// Operator <<
std::ostream& operator << (std::ostream& os, const Gate& inst) {
  os << inst.cell << ' ' << inst.name << " ( ";
  size_t i {0};
  for(const auto& [pin, net] : inst.cellpin2net) {
    if(i++ != 0) {
      os << ", ";
    }
    os << '.' << pin << '(' << net << ')';
  }
  os << " );";
  return os;
}

// ------------------------------------------------------------------------------------------------

// Operator: <<
std::ostream& operator << (std::ostream& os, const Module& m) {

  os << "module " << m.name << " (\n";  

  for(size_t i=0; i<m.ports.size(); ++i) {
    os << m.ports[i];
    if(i != m.ports.size() - 1) {
      os << ",\n";
    }
  }
  os << ");\n\n";

  os << "// Start PIs\n";
  for(const auto& pi : m.inputs) {
    os << "input " << pi << ";\n";
  }
  os << '\n';
  
  os << "// Start POs\n";
  for(const auto& po : m.outputs) {
    os << "output " << po << ";\n";
  }
  os << '\n';
  
  os << "// Start wires\n"; 
  for(const auto& wire : m.wires) {
    os << "wire " << wire << ";\n";
  }
  os << '\n';

  os << "// Start cells\n";
  for(const auto& gate : m.gates) {
    os << gate << '\n';
  }
  os << '\n';

  os << "endmodule";
  
  return os;
}

// ------------------------------------------------------------------------------------------------

// Function: info
std::string Module::info() const {
  return "verilog module \""s + name + "\" " + 
         "[gates:" + std::to_string(gates.size()) + "]";
}

// Procedure: read_verilog
Module read_verilog(const std::filesystem::path& path) {

  Module module;
  
  static std::string_view delimiters = "(),:;/#[]{}*\"\\";
  static std::string_view exceptions = "().:;[]";
  
  auto tokens = tokenize(path, delimiters, exceptions);

  for (const auto& token : tokens) {
   std::cout << token << std::endl;
  }

  // Set up the iterator
  auto itr = tokens.begin();
  auto end = tokens.end();

  // Read the module name
  if(itr = std::find(itr, end, "module"); itr == end) {
    OT_LOGF("can't find keyword 'module'");
  }
  else {
    if(++itr == end) {
      OT_LOGF("syntax error in module name");
    }
    module.name = std::move(*itr);
  }

  for (auto token: tokens) {
    OT_LOGD("   Token: ", token);
  }

  while(++itr != end && *itr != ";") {
    if(*itr != "(" && *itr != ")") {
      // OT_LOGD("Ports:  ", *itr);
      module.ports.push_back(std::move(*itr));
    }
  }

  // Parse the content.
  while(++itr != end) {
    
    if(*itr == "endmodule") {
      break;
    }
    else if(*itr == "input") {
      itr++; // "["
      if (*itr == "[") {
        itr++; // bus size
        int bus_size = std::stoi(*itr);
        if (bus_size <= 0) OT_LOGE("Bus size <= 0!!");
        itr++; // ":"
        itr++; // "0"
        if (*itr != "0") OT_LOGE("LSB is not 0!!");
        itr++; // "]"
        itr++; // input name
        for (int bus_idx = bus_size; bus_idx >= 0; bus_idx--) {
          std::string input_name = *itr + "_" + std::to_string(bus_idx);
          OT_LOGD("ADD input: ", input_name);
          module.inputs.push_back(input_name);
        }
        itr++; // ";"
      }
      else {
        while(itr != end && *itr != ";") {
          OT_LOGD("ADD input: ", *itr);
          module.inputs.push_back(std::move(*itr));
          itr++;
        }
      }
      // original code
      // while(++itr != end && *itr != ";") {
      //   module.inputs.push_back(std::move(*itr));
      // }
    }
    else if(*itr == "output") {
      itr++; // "["
      if (*itr == "[") {
        itr++; // bus size
        int bus_size = std::stoi(*itr);
        if (bus_size <= 0) OT_LOGE("Bus size <= 0!!");
        itr++; // ":"
        itr++; // "0"
        if (*itr != "0") OT_LOGE("LSB is not 0!!");
        itr++; // "]"
        itr++; // input name
        for (int bus_idx = bus_size; bus_idx >= 0; bus_idx--) {
          std::string output_name = *itr + "_" + std::to_string(bus_idx);
          OT_LOGD("ADD output: ", output_name);
          module.outputs.push_back(output_name);
        }
        itr++; // ";"
      } else {
        while(itr != end && *itr != ";") {
          OT_LOGD("ADD output: ", *itr);
          module.outputs.push_back(std::move(*itr));
          itr++;
        }
      }
      // original code
      // while(++itr != end && *itr != ";") {
      //   module.outputs.push_back(std::move(*itr));
      // }
    }
    else if(*itr == "wire") {
      itr++; // "["
      if (*itr == "[") {
        itr++; // bus size
        int bus_size = std::stoi(*itr);
        if (bus_size <= 0) OT_LOGE("Bus size <= 0!!");
        itr++; // ":"
        itr++; // "0"
        if (*itr != "0") OT_LOGE("LSB is not 0!!");
        itr++; // "]"
        itr++; // input name
        for (int bus_idx = bus_size; bus_idx >= 0; bus_idx--) {
          std::string wire_name = *itr + "_" + std::to_string(bus_idx);
          OT_LOGD("ADD wire: ", wire_name);
          module.wires.push_back(wire_name);
        }
        itr++; // ";"
      } else {
        while(itr != end && *itr != ";") {
          OT_LOGD("ADD wire: ", *itr);
          module.wires.push_back(std::move(*itr));
          itr++;
        }
      }
      // original
      // while(++itr != end && *itr != ";") {
      //   std::vector<std::string>::iterator w_itr = std::find(module.wires.begin(), module.wires.end(), *itr);
      //   if (w_itr != module.wires.cend()) {
      //     OT_LOGD("Element present at index ", std::distance(module.wires.begin(), w_itr));
      //   }
      //   OT_LOGD("ADD wire: ", *itr);
      //   module.wires.push_back(std::move(*itr));
      // }
    }
    else {
      
      Gate inst;
      inst.cell = std::move(*itr);

      if(++itr == end) {
        OT_LOGF("syntax error in cell ", inst.cell, ")");
      }
      inst.name = std::move(*itr);

      // Read the mapping
      std::string cellpin;
      std::string net;

      // original
      // itr = on_next_parentheses(itr, end, [&] (auto& str) mutable { 
      //   if(str == ")" || str == "(") {
      //     return;
      //   }
      //   else if(str[0] == '.') {
      //     cellpin = str.substr(1);
      //   }
      //   else {
      //     net = str;
      //     OT_LOGE(inst.name, "  ", cellpin, "  ", net);
      //     inst.cellpin2net[cellpin] = net;
      //     inst.net2cellpin[net] = cellpin;
      //   }
      // });

      auto l_itr = std::find(itr, end, "(");
      auto r_itr = l_itr;
      int stack = 0;

      while(r_itr != end) {
        if(*r_itr == "(") {
          ++stack;
        }
        else if(*r_itr == ")") {
          --stack;
        }
        if(stack == 0) {
          break;
        }
        ++r_itr;
      }
      
      if (l_itr == end || r_itr == end) continue;;

      for (++l_itr; l_itr != r_itr; ++l_itr) {
        std::string str = *l_itr;
        if(str == ")" || str == "(") {
          continue;;
        }
        else if(str[0] == '.') {
          cellpin = str.substr(1);
        }
        else {
          if (*(++l_itr) == "[") {
            net = str + "_" + *(++l_itr);
            l_itr++;
          } else {
            net = str;
          }
          OT_LOGE(inst.name, "  ", cellpin, "  ", net);
          inst.cellpin2net[cellpin] = net;
          inst.net2cellpin[net] = cellpin;
        }
      }

      itr = r_itr;

      if(itr == end) {
        OT_LOGF("syntax error in gate pin-net mapping");
      }

      if(*(++itr) != ";") {
        OT_LOGF("missing ; in instance declaration");
      }
      
      module.gates.push_back(std::move(inst));
    }
  }
  
  return module;
}

};  // end of namespace ot. -----------------------------------------------------------------------

set_input_delay 0 -min -rise [get_ports {test[2]}]
set_input_delay 0 -min -fall [get_ports {test[2]}]
set_input_delay 0 -max -rise [get_ports {test[2]}]
set_input_delay 0 -max -fall [get_ports {test[2]}]
set_input_transition 5 -min -rise [get_ports {test[2]}]
set_input_transition 5 -min -fall [get_ports {test[2]}]
set_input_transition 5 -max -rise [get_ports {test[2]}]
set_input_transition 5 -max -fall [get_ports {test[2]}]
set_input_delay 0 -min -rise [get_ports {test[1]}]
set_input_delay 0 -min -fall [get_ports {test[1]}]
set_input_delay 0 -max -rise [get_ports {test[1]}]
set_input_delay 0 -max -fall [get_ports {test[1]}]
set_input_transition 5 -min -rise [get_ports {test[1]}]
set_input_transition 5 -min -fall [get_ports {test[1]}]
set_input_transition 5 -max -rise [get_ports {test[1]}]
set_input_transition 5 -max -fall [get_ports {test[1]}]
set_input_delay 0 -min -rise [get_ports {test[0]}]
set_input_delay 0 -min -fall [get_ports {test[0]}]
set_input_delay 0 -max -rise [get_ports {test[0]}]
set_input_delay 0 -max -fall [get_ports {test[0]}]
set_input_transition 5 -min -rise [get_ports {test[0]}]
set_input_transition 5 -min -fall [get_ports {test[0]}]
set_input_transition 5 -max -rise [get_ports {test[0]}]
set_input_transition 5 -max -fall [get_ports {test[0]}]
set_input_delay 0 -min -rise [get_ports nx2]
set_input_delay 0 -min -fall [get_ports nx2]
set_input_delay 0 -max -rise [get_ports nx2]
set_input_delay 0 -max -fall [get_ports nx2]
set_input_transition 5 -min -rise [get_ports nx2]
set_input_transition 5 -min -fall [get_ports nx2]
set_input_transition 5 -max -rise [get_ports nx2]
set_input_transition 5 -max -fall [get_ports nx2]
set_input_delay 0 -min -rise [get_ports nx6]
set_input_delay 0 -min -fall [get_ports nx6]
set_input_delay 0 -max -rise [get_ports nx6]
set_input_delay 0 -max -fall [get_ports nx6]
set_input_transition 5 -min -rise [get_ports nx6]
set_input_transition 5 -min -fall [get_ports nx6]
set_input_transition 5 -max -rise [get_ports nx6]
set_input_transition 5 -max -fall [get_ports nx6]
create_clock -period 100 -name virtual_clock
set_output_delay -9 -min -rise [get_ports nx23] -clock virtual_clock
set_output_delay -9 -min -fall [get_ports nx23] -clock virtual_clock
set_output_delay 89 -max -rise [get_ports nx23] -clock virtual_clock
set_output_delay 89 -max -fall [get_ports nx23] -clock virtual_clock
set_load -pin_load 4 [get_ports nx23]
set_output_delay -9 -min -rise [get_ports nx22] -clock virtual_clock
set_output_delay -9 -min -fall [get_ports nx22] -clock virtual_clock
set_output_delay 89 -max -rise [get_ports nx22] -clock virtual_clock
set_output_delay 89 -max -fall [get_ports nx22] -clock virtual_clock
set_load -pin_load 4 [get_ports nx22]

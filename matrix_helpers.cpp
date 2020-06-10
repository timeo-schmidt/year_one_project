#include "dependencies.hpp"
#include "simulator.hpp"

bool is_a_node_voltage_known(node input, node reference_node) {
  // check if a node is connected to any voltage sources, if so, check whether the other node of the voltage source is the reference node, or connected to another voltage source.

  if(input == reference_node){
    return true;
  }

  for(int i = 0 ; i < input.connected_components.size(); i++) {
    if(input.connected_components[i].component_name.at(0) == 'V') {
      if(input.connected_components[i].connected_terminals[0] == reference_node){
        return true;
      }
      if(input.connected_components[i].connected_terminals[0] == input) {
        if(input.connected_components[i].connected_terminals[1] == reference_node){
          return true;
        }
        else{
          //the following part might be a bit wrong, because the function might go back to the input node during recursion
          if(input.connected_components[i].connected_terminals[0]==input) {
            return is_a_node_voltage_known(input.connected_components[i].connected_terminals[1], reference_node);
          }
          if(input.connected_components[i].connected_terminals[1]==input) {
            return is_a_node_voltage_known(input.connected_components[i].connected_terminals[0], reference_node);
          }
        }
      }
    }
  }
  return false;

}


bool r_two_nodes_supernodes(component cmp, node reference_node) {
  // this bool function checks if two nodes should be combined into supernodes, thus resulting in a different value in the current column
  // supernodes should be represented by two rows in the matrix.
  // the first row shows the relationship between the two nodes.
  // the second row shows the sum of the conductance terms of two nodes.
  node node1(1);
  node1 = cmp.connected_terminals[0];
  node node2(2);
  node2 = cmp.connected_terminals[1];

  if(is_a_node_voltage_known(node1, reference_node) == 0 && is_a_node_voltage_known(node2, reference_node)== 0 ){
    return true;
  }
  return false;
}

double sum_known_currents(node input, double simulation_progress) {
//this function sums up the currents going out of one node at a specific time
	double sum_current = 0.0;
	for(int i = 0 ; i < input.connected_components.size() ; i++){
		if(input.connected_components[i].component_name[0] == 'I'){
			if(input.connected_components[i].connected_terminals[0] == input) {
				sum_current += input.connected_components[i].component_value[0]; // add dc offset
				sum_current += input.connected_components[i].component_value[1]* sin(input.connected_components[i].component_value[2] * simulation_progress); // add amplitude*sin(frequency*time
			}
			if(input.connected_components[i].connected_terminals[1] == input) {
				sum_current -= input.connected_components[i].component_value[0]; // subtract dc offset
				sum_current -= input.connected_components[i].component_value[1]* sin(input.connected_components[i].component_value[2] * simulation_progress); // subtract amplitude*sin(frequency*time
			}
		}
	}
	return sum_current;
}

// Returns pairs of normal nodes, which represent a supernode.
vector<pair<node,node>> supernode_separation(vector<component> components, node reference_node) {
  // A supernode consists of two nodes. In the matrix a supernode occupies two lines. Each of the two nodes is separated
  //this function takes in the components in the network simulation and checks supernodes.
  //This function outputs a vector of vector of nodes.
  //The first vector of nodes contains the relationship node in the supernode, e.g. 1*V2 - 1*V3 = 10
  //The second vector of nodes contains the non relationship node in the supernode, e.g. G11+G21, G12+G22, G13+G23 row
  //This function finds the v sources and does supernode separation

	vector<node> relationship_node;
	vector<node> non_relationship_node;
  vector<pair<node,node>> output;
  for(component cmp: components) {
		if(cmp.component_name[0] == 'V'){
			if(r_two_nodes_supernodes(cmp, reference_node)){
				//the positive side of the V source is going to be the relationship node (assumed to be, it doesnt matter if it's the relationship one or the non relationship one
        output.push_back({cmp.connected_terminals[0], cmp.connected_terminals[1]});
			}
		}
	}
	return output;
}



//!!!!!  Here the first term in connected_terminals vector of the components is assumed to be positive, the second one is negative    !!!!!!

//capacitors need to be replaced by voltage sources at different time steps.
//capacitors are closed circuit in the beginning.
independent_v_source convert_C_to_vsource(C capacitor, double timestep){

}

//inductors are open circuits in the beginning.
//inductors need to be replaced by current sources at different time steps.
independent_i_source convert_L_to_isource(L inductor, double timestep){

	//a timer is needed to produce the current_time in the simulation
	//to use this function, u have to make sure the inductor's connected_terminals's voltages are updated after each time step
	//when simulation starts, L should not be converted to an i source
	double voltage_across_L = inductor.connected_terminals[0].node_voltage - inductor.connected_terminals[1].node_voltage;
	double isource_value;
	isource_value = (voltage_across_L / inductor.component_value[0])*timestep;
	independent_i_source Ltoisource("Ctois", isource_value, 0.0, 0.0, inductor.connected_terminals);
	return Ltoisource;

}

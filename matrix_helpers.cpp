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

  if(is_a_node_voltage_known(node1, reference_node) == 0 && is_a_node_voltage_known(node2, reference_node)== 0){
    return true;
  }
  return false;
}

double sum_known_currents(node input, double simulation_progress) {
//this function sums up the currents going out of one node at a specific time
	double sum_current = 0.0;
	for(int i = 0 ; i < input.connected_components.size() ; i++){
		if(input.connected_components[i].component_name[0] == 'I'){
			if(input.connected_components[i].connected_terminals[1] == input) {
				sum_current += input.connected_components[i].component_value[0]; // add dc offset
				sum_current += input.connected_components[i].component_value[1]* sin(2*M_PI*input.connected_components[i].component_value[2] * simulation_progress); // add amplitude*sin(2*pi*frequency*time
			}
			if(input.connected_components[i].connected_terminals[0] == input) {
				sum_current -= input.connected_components[i].component_value[0]; // subtract dc offset
				sum_current -= input.connected_components[i].component_value[1]* sin(2*M_PI*input.connected_components[i].component_value[2] * simulation_progress); // subtract amplitude*sin(2*pi*frequency*time
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

int which_is_the_node(vector<node> nodes_wo_ref , node input){
	for(int i = 0; i < nodes_wo_ref.size(); i++){
		if(nodes_wo_ref[i] == input){
			return i;
		}
	}
  // Node not found. Must be reference.
  return -1;
}

int which_is_cmp(vector<component> networkcmp, component input){
  for(int i = 0 ; i < networkcmp.size(); i++){
    if(networkcmp[i].component_name[0] == input.component_name[2] && networkcmp[i].component_name[1] == input.component_name[3]){
      return i;
    }
  }
  // Component not found.
  return -1;
}

int which_is_cmp1(vector<component> networkcmp, component input){
     for(int i = 0 ; i < networkcmp.size(); i++){
       if(networkcmp[i].component_name == input.component_name){
         return i;
       }
     }
     // Component not found.
     return -1;
   
}

void update_source_equivalents(network_simulation &sim, vector<node> Vvector, vector<double> current_through_components, double simulation_progress, double timestep){

//  vector<component> network_components = sim.network_components;

  for(int i = 0 ; i < sim.network_components.size(); i++){
    if(sim.network_components[i].component_name[1] == '_') {

      // The node voltages with a 0 current source are correct. However, they seem wrong here.
      if(sim.network_components[i].component_name[0] == 'I') {
      	int which_is_node0 = which_is_the_node(Vvector, sim.network_components[i].connected_terminals[0]);
      	int which_is_node1 = which_is_the_node(Vvector, sim.network_components[i].connected_terminals[1]);

        double voltage_across_component;
        if(which_is_node0 != -1) {
          voltage_across_component = Vvector[which_is_node0].node_voltage - Vvector[which_is_node1].node_voltage;
        } else if(which_is_node0==-1) {
          voltage_across_component = 0 - Vvector[which_is_node1].node_voltage;
        } else if(which_is_node1==-1) {
          voltage_across_component = Vvector[which_is_node0].node_voltage - 0;
        } else {
          voltage_across_component = 0.0;
        }
 

        sim.network_components[i].component_value[0]=(voltage_across_component / sim.cl_values[sim.network_components[i].component_name])*timestep + sim.network_components[i].component_value[0];
		independent_i_source equivalent_source(sim.network_components[i].component_name, sim.network_components[i].component_value[0], 0.0, 0.0, sim.network_components[i].connected_terminals);
		sim.network_components[i] = equivalent_source;

 		int which0  = which_is_the_node(sim.network_nodes, sim.network_components[i].connected_terminals[0]);
        int which1  = which_is_the_node(sim.network_nodes, sim.network_components[i].connected_terminals[1]);
		int node_cmp_idx1 = which_is_cmp1(sim.network_nodes[which0].connected_components, sim.network_components[i]);
        sim.network_nodes[which0].connected_components[node_cmp_idx1] = equivalent_source;

        int node_cmp_idx2 = which_is_cmp1(sim.network_nodes[which1].connected_components, sim.network_components[i]);
        sim.network_nodes[which1].connected_components[node_cmp_idx2] = equivalent_source;


      }

      if(sim.network_components[i].component_name[0] == 'V'){
		
 		int which_is_node0 = which_is_the_node(Vvector, sim.network_components[i].connected_terminals[0]);
      	int which_is_node1 = which_is_the_node(Vvector, sim.network_components[i].connected_terminals[1]);

      	double current_across_component = tell_currents(sim.network_components[i], Vvector, simulation_progress);
		
		sim.network_components[i].component_value[0] = (-current_across_component / sim.cl_values[sim.network_components[i].component_name])*timestep + sim.network_components[i].component_value[0];
		independent_v_source equivalent_source(sim.network_components[i].component_name, sim.network_components[i].component_value[0], 0.0, 0.0, sim.network_components[i].connected_terminals);


		sim.network_components[i] = equivalent_source;
 
		int which0  = which_is_the_node(sim.network_nodes, sim.network_components[i].connected_terminals[0]);
		int which1  = which_is_the_node(sim.network_nodes, sim.network_components[i].connected_terminals[1]);
		int node_cmp_idx1 = which_is_cmp1(sim.network_nodes[which0].connected_components, sim.network_components[i]);
        sim.network_nodes[which0].connected_components[node_cmp_idx1] = equivalent_source;
		
        int node_cmp_idx2 = which_is_cmp1(sim.network_nodes[which1].connected_components, sim.network_components[i]);
        sim.network_nodes[which1].connected_components[node_cmp_idx2] = equivalent_source;
		
		}

	 }

  }
}



void convert_CLs_to_sources(network_simulation &sim){

  for(int i = 0 ; i < sim.network_components.size(); i++){
    if(sim.network_components[i].component_name[0] == 'L'){
      sim.cl_values.insert(make_pair("I_"+sim.network_components[i].component_name, sim.network_components[i].component_value[0]));

      independent_i_source equivalent_source("I_"+sim.network_components[i].component_name, 0.0, 0.0, 0.0, sim.network_components[i].connected_terminals);
      sim.network_components[i] = equivalent_source;

      int which0 = which_is_the_node(sim.network_nodes,sim.network_components[i].connected_terminals[0]);
      int which1 = which_is_the_node(sim.network_nodes,sim.network_components[i].connected_terminals[1]);


      int node_cmp_idx1 = which_is_cmp(sim.network_nodes[which0].connected_components, sim.network_components[i]);
      sim.network_nodes[which0].connected_components[node_cmp_idx1] = sim.network_components[i];

      int node_cmp_idx2 = which_is_cmp(sim.network_nodes[which1].connected_components, sim.network_components[i]);
      sim.network_nodes[which1].connected_components[node_cmp_idx2] = sim.network_components[i];

    }
    else if(sim.network_components[i].component_name[0] == 'C'){
      sim.cl_values.insert(make_pair("V_"+sim.network_components[i].component_name, sim.network_components[i].component_value[0]));

      independent_v_source equivalent_source("V_"+sim.network_components[i].component_name, 0.0, 0.0, 0.0, sim.network_components[i].connected_terminals);
      sim.network_components[i] = equivalent_source;

      int which0 = which_is_the_node(sim.network_nodes,sim.network_components[i].connected_terminals[0]);
      int which1 = which_is_the_node(sim.network_nodes,sim.network_components[i].connected_terminals[1]);


      int node_cmp_idx1 = which_is_cmp(sim.network_nodes[which0].connected_components, sim.network_components[i]);
      sim.network_nodes[which0].connected_components[node_cmp_idx1] = sim.network_components[i];

      int node_cmp_idx2 = which_is_cmp(sim.network_nodes[which1].connected_components, sim.network_components[i]);
      sim.network_nodes[which1].connected_components[node_cmp_idx2] = sim.network_components[i];

    }
  }

}

//the following function is used to tell the currents through a V source
//it calculates the current resulted from other components (not from the source itself) from connected_terminals[0] to connected_terminals[1]
//output increases when current goes out of terminal[0] , output decreases when current goes out of terminal[1]
double tell_currents(component input, vector<node> Vvector, double simulation_progress){
	double output = 0.0;
	//sum currents going out of terminal[0]
	int which_is_node0 = which_is_the_node(Vvector, input.connected_terminals[0]);
	int which_is_node1 = which_is_the_node(Vvector, input.connected_terminals[1]);
	if(input.connected_terminals[1].index == 0){ // if input.connected_terminals[1] is not in Vvector, this is because Vvector doesn't contain the reference node.
		for(int i = 0 ; i < Vvector[which_is_node0].connected_components.size() ; i++){

			if(Vvector[which_is_node0].connected_components[i].component_name[0] == 'R'){
				if(Vvector[which_is_node0].connected_components[i].connected_terminals[0] == Vvector[which_is_node0]){
					output += calculate_current_through_R(Vvector[which_is_node0].connected_components[i], Vvector);
				}
				if(Vvector[which_is_node0].connected_components[i].connected_terminals[1] == Vvector[which_is_node0]){
					output -= calculate_current_through_R(Vvector[which_is_node0].connected_components[i], Vvector);
				}
			}
			if(Vvector[which_is_node0].connected_components[i].component_name[0] == 'I'){
				if(Vvector[which_is_node0].connected_components[i].connected_terminals[0] == Vvector[which_is_node0]){
					output += Vvector[which_is_node0].connected_components[i].component_value[0] + Vvector[which_is_node0].connected_components[i].component_value[1]*sin(2*M_PI*Vvector[which_is_node0].connected_components[i].component_value[2]*simulation_progress);
				}
				if(Vvector[which_is_node0].connected_components[i].connected_terminals[1] == Vvector[which_is_node0]){
					output -= Vvector[which_is_node0].connected_components[i].component_value[0] + Vvector[which_is_node0].connected_components[i].component_value[1]*sin(2*M_PI*Vvector[which_is_node0].connected_components[i].component_value[2]*simulation_progress);
				}
			}
		}

	} else {
		for(int c = 0; c< Vvector[which_is_node1].connected_components.size(); c++){
			if(Vvector[which_is_node1].connected_components[c].component_name[0] == 'R'){
				if(Vvector[which_is_node1].connected_components[c].connected_terminals[0] == Vvector[which_is_node1]){
					output -= calculate_current_through_R(Vvector[which_is_node1].connected_components[c], Vvector);
				}
				if(Vvector[which_is_node1].connected_components[c].connected_terminals[1] == Vvector[which_is_node1]){
					output += calculate_current_through_R(Vvector[which_is_node1].connected_components[c], Vvector);
				}
			}
			if(Vvector[which_is_node1].connected_components[c].component_name[0] == 'I'){
				if(Vvector[which_is_node1].connected_components[c].connected_terminals[0] == Vvector[which_is_node1]){
					output -= Vvector[which_is_node1].connected_components[c].component_value[0] + Vvector[which_is_node1].connected_components[c].component_value[1]*sin(2*M_PI*Vvector[which_is_node1].connected_components[c].component_value[2]*simulation_progress);
				}
				if(Vvector[which_is_node1].connected_components[c].connected_terminals[1] == Vvector[which_is_node1]){
					output += Vvector[which_is_node1].connected_components[c].component_value[0] + Vvector[which_is_node1].connected_components[c].component_value[1]*sin(2*M_PI*Vvector[which_is_node1].connected_components[c].component_value[2]*simulation_progress);
				}
			}

		}
	}

	return output;
}


double calculate_current_through_R(component R, vector<node> Vvector){
	//the function calculates the current through R by using the node voltage difference across it divided by R value
	// this function is written because nodes are recreated in Vvector when copying from the network_nodes of the network simulation class
	double node0voltage = 0.0;
	double node1voltage = 0.0;
	double current_through_R;
	for(node no: Vvector){
		if(R.connected_terminals[0].index == no.index){
			node0voltage = no.node_voltage;
		}
		if(R.connected_terminals[1].index == no.index){
			node1voltage = no.node_voltage;
		}
	}
	current_through_R = (node0voltage - node1voltage) / R.component_value[0];
	return current_through_R;

}
//the following function should take the version of network_component, where all C and Ls are converted to sources.
//the output of the function includes the current through all components from the input. The orders are matched.
vector<double> calculate_current_through_component(vector<component> network_component, vector<node> Vvector, double simulation_progress){

	vector<double> current_column;

	//go through all components
	//treat different ones differently
	for(int i = 0 ; i < network_component.size() ; i++){

		// the current through a resistor is done by ( the node voltage at connected_terminals[0] - the node voltage at connected_terminals[1]) / resistor value.
		// Also, the i sources connected to the connected_terminals of the resistor are taken into account.
		// To keep it consistent, its always positive.
		if(network_component[i].component_name[0] == 'R'){
			double current_through_R = 0.0;
			current_through_R = calculate_current_through_R(network_component[i], Vvector);
			current_column.push_back(current_through_R);
		}

		// The current through V shows the current going through V from the positive side of the v source to the negative side of the v source
		if(network_component[i].component_name[0] == 'V'){
			double current_through_V = 0.0;
			current_through_V = tell_currents(network_component[i],Vvector, simulation_progress);
			current_column.push_back(current_through_V);
		}

		//The current through I shows the current going through I from the In side to the Out side.
		if(network_component[i].component_name[0] == 'I'){
			double current_through_I = 0.0;
			current_through_I = network_component[i].component_value[0] + network_component[i].component_value[1]*sin(2*M_PI*network_component[i].component_value[2]*simulation_progress);
			current_column.push_back(current_through_I);
		}

	}
	return current_column;

}

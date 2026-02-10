// Here we design an implementation of the shortest path problem with resource constraints (SPPRC) using dynamic programming.
// The SPPRC is a generalization of the shortest path problem where we have additional constraints on the resources consumed along the path.
#include <iostream>
#include <vector>
#include <algorithm>
#include <queue>

// Basic structure: arcs in the graph
struct Arc {
    int from;
    int to;
    double cost;
    std::vector<int> resource_consumption;  // e.g., time, load, etc.
};

// Basic structures for SPPRC
struct Label {
    int label_id;
    int node;
    std::vector<int> resources;  // e.g., time, load, etc.
    double cost;
    int prev_label_id;      // for path reconstruction
    bool dominated;
    std::vector<int> unreachable_nodes;  // optional: for ng-route relaxation
};

struct ResourceWindow {
    double min_val, max_val;
    bool is_continuous;
};


class SPPRC {
private:
    std::vector<std::vector<Label>> all_labels;  // labels per node as a matrix
    std::vector<ResourceWindow> resource_limits;
    std::vector<std::vector<std::pair<int, Arc>>> graph;  // adjacency list
    
    // Dominance comparison
    bool dominates(const Label& l1, const Label& l2) {
        if (l1.cost > l2.cost) return false;
        for (int i = 0; i < resource_limits.size(); i++) {
            if (l1.resources[i] > l2.resources[i]) return false;
        }
        return true;
    }

    // feasibility check
    bool isFeasible(const Label& label) {
        for (int i = 0; i < resource_limits.size(); i++) {
            if (label.resources[i] > resource_limits[i].max_val) return false;
            if (label.resources[i] < resource_limits[i].min_val) return false;
        }
        return true;
    }

    //dominance check before adding new label
    bool isDominated(const Label& new_label, int node) {
        for (const Label& existing : all_labels[node]) {
            if (dominates(existing, new_label)) {
                return true;  // new label is dominated
            }
        }
        return false;  // not dominated
    }

    // Remove labels that are dominated by the new label
    void removeDominatedLabels(int node, const Label& new_label) {
        std::vector<Label>& labels = all_labels[node];
        labels.erase(std::remove_if(labels.begin(), labels.end(),
            [&](const Label& existing) {
                return dominates(new_label, existing);
            }), labels.end());
    }

    bool extendLabel(int from_node, int to_node, const Arc& arc) {
        for (const Label& label : all_labels[from_node]) {
            if (label.dominated) continue;
            
            Label new_label = label;
            new_label.node = to_node;
            new_label.cost += arc.cost;
            
            // Update resources
            for (int i = 0; i < label.resources.size(); i++) {
                new_label.resources[i] += arc.resource_consumption[i];
            }
            
            // Check feasibility
            if (isFeasible(new_label)) {
                // Apply dominance check before adding
                if (!isDominated(new_label, to_node)) {
                    all_labels[to_node].push_back(new_label);
                    removeDominatedLabels(to_node);
                }
            }
        }
    }

    // initial label creation
    Label createInitialLabel(int source) {
        Label label;
        label.label_id = 0;
        label.node = source;
        label.resources = std::vector<int>(resource_limits.size(), 0);
        label.cost = 0.0;
        label.prev_label_id = -1;  // no predecessor
        label.dominated = false;
        return label;
    }

    // main dominance loop
    void applyDominance(int node) {
        std::vector<Label>& labels = all_labels[node];
        for (size_t i = 0; i < labels.size(); i++) {
            if (labels[i].dominated) continue;
            for (size_t j = i + 1; j < labels.size(); j++) {
                if (labels[j].dominated) continue;
                if (dominates(labels[i], labels[j])) {
                    labels[j].dominated = true;
                } else if (dominates(labels[j], labels[i])) {
                    labels[i].dominated = true;
                    break;  // no need to check further for label i
                }
            }
        }
    }

    void solve(int source, int sink) {
        // Priority queue or simple queue (depending on resource types)
        std::queue<int> active_nodes;
        Label initial_label = createInitialLabel(source);
        all_labels[source].push_back(initial_label);
        active_nodes.push(source);
        
        while (!active_nodes.empty()) {
            int current = active_nodes.front();
            active_nodes.pop();
            
            for (const auto& [next_node, arc] : graph[current]) {
                extendLabel(current, next_node, arc);
            }
            
            applyDominance(current);
        }
    }
};


from pyeda.inter import *   # Needed for BDDs
from Node import *
'''
Author: Beth Ernst - Macalester College - eernst@macalester.edu
Date: 8/5/19

This is a data structure file.  Intended to be used and run from main.py

The network structure is used to keep track of all the nodes contained in the network while a node substructure is
used to maintain the more detailed information for the individual nodes in the network. Most of the interconnection
information can be maintained using this node substructure leaving the network structure only needing to store the nodes
as three sets: gate nodes that represent the output functions, input nodes that represent the primary inputs, and all of
the remaining gate nodes in the network, the internal nodes. The network data structure will also store the value of the
cost function for the network it describes.

Pseudocode for the network data structure:

Structure Network {
     Set OutputNodes;  //Set of Nodes representing output functions
     Set InputNodes;   //Set of Nodes representing input variables
     Set InternalNodes; //Set of Nodes representing gates
     Int Cost;  //Number of gate nodes
}
'''


class Network:
    """ Network class for representing a network of nodes """

    def __init__(self):
        self.idx = 0   # Keep track of levels of networks
        self.outputs = [] # List of nodes representing the output functions
        self.inputs = [] # List of nodes representing the input functions
        self.internal = [] # List of nodes represeting the internal gates
        self.cost = 0 # current cost of the network
        self.vars = 0 # Number of input variables for the network

    def CopyNetwork(self):
        """
        Makes a duplicate network
        """
        newNet = Network()
        mapping = {} # A mapping from old node -> new node to help with network connections

        # Copy Inputs
        for node in self.inputs:
            newNode = newNet.InsertInput(node.on)
            mapping[node] = newNode

        # Copy Outputs
        for node in self.outputs:
            newNode = newNet.InsertOutput(node.on)
            mapping[node] = newNode

        # Copy Internal
        for node in self.internal:
            newNode = newNet.InsertBlankInternal()
            newNode.on = node.on
            newNode.off = node.off
            newNode.uncovered = node.uncovered
            mapping[node] = newNode

        # Copy Symmetries - SKIPPING FOR NOW

        # Update fan-in and fan-out of output nodes
        for node in self.outputs:
            newNode = mapping[node]
            for fo in node.fan_out:
                newNode.fan_out.append(mapping[fo])
            for fi in node.fan_in:
                newNode.fan_in.append(mapping[fi])

        # Update fan-in and fan-out of internal nodes
        for node in self.internal:
            newNode = mapping[node]
            for fo in node.fan_out:
                newNode.fan_out.append(mapping[fo])
            for fi in node.fan_in:
                newNode.fan_in.append(mapping[fi])

        # Update fan-out of input nodes
        for node in self.inputs:
            newNode = mapping[node]
            for fo in node.fan_out:
                newNode.fan_out.append(mapping[fo])
            for fi in node.fan_in:
                newNode.fan_in.append(mapping[fi])

        # Update index
        newNet.idx = self.idx + 1
        # Update Cost
        newNet.cost = self.cost

        return (newNet, mapping)

    def InsertInput(self, f):
        """Inserts a new input node into the network with function f"""
        n = Node()
        n.idx =self.vars
        self.vars += 1
        n.level = -1  # inputs have level -1
        n.on = f
        n.off = ~f
        # n.uncovered stays at 0
        # n.fan_in stays empty
        # n.fan_out stays empty
        # n.connectible stays empty
        n.logicOp = 'I'

        self.inputs.append(n)  # Add n to the list of Input nodes
        return n

    def InsertOutput(self, f, logicOp='-'):
        """ Inserts a new output node into the network with function f """
        n = Node()
        n.idx = self.vars
        self.vars += 1
        n.level = 0
        n.on = f
        n.off = ~f
        # n.fan_in stays empty
        # n.fan_out stays empty
        # n.connectible stays empty
        if GATE_TYPE == 1:
            n.uncovered = f
            n.logic_op = 'D'
        elif GATE_TYPE == 2:
            n.uncovered = ~f
            n.logic_op = 'R'

        self.cost += 1
        self.outputs.append(n)  # Add n to the list of Output nodes
        return n

    def InsertBlankInternal(self, logicOp='-'):
        # Create internal node
        n = Node()
        n.idx = self.vars
        self.vars += 1
        n.level = 0
        if GATE_TYPE == 1:
            n.logic_op = 'D'
        elif GATE_TYPE == 2:
            n.logic_op = 'R'
        self.cost += 1
        self.internal.append(n)  # Add n to the list of internal nodes
        return n

    def Initialization(self, inputBDDs, outputBDD):
        # Add input nodes
        for i in inputBDDs:
            self.InsertInput(i)

        # Add output node
        self.InsertOutput(outputBDD)

        # Compute initial cost
        self.cost = self.ComputeCost()

        # Update connectible sets
        self.UpdateConnectibleSets()

        # Do Implications by adding new gates now
        # only works for nand only or nor only networks
        if GATE_TYPE == 1 or GATE_TYPE == 2:
            self.AddNewNodes()

    '''The AddNewNode procedure detects and performs one structural implication that exists in a partial network.
    The first step will be to detect if there exists a node in the network that cannot be completely covered by existing 
    nodes. The procedure will detect all such nodes and then add a new node to the partial network to cover one of them.
    The covering is done as in the PerformCovering function.The new gate node is added to the network and is then used 
    to cover a minterm from the selected node that cannot be covered by any existing node.
    
    AddNewGate (Network) {
        //Detect uncoverable node
        for (all gate nodes N ∈ Network) {
            Uncoverable ← N.uncovered;
            for (all nodes C ∈ N.fan_in ∪ N.connectible_set) {
                Uncoverable ← Uncoverable * C.on; 
            }
            if (Uncoverable ≠ 0)
                Store N as the uncovered node;
        }
        // Perform Covering
        Create a new gate node C in Network; N.fan_in ← N.fan_in ∪ {C}; C.fan_out ← C.fan_out ∪ {N};
        C.off ← SelectMinterm (Uncoverable); FunctionalImplication(C); FunctionalImplication(N);
    }
    '''
    def AddNewNodes(self):
        """
        Performs structural implications.
        If there is a node in the network that cannot be covered by an existing node then new nodes will
        be added until all nodes can be covered by existing nodes.
        """
        newNodeNeeded, uncovered = self.FindNewNodeNeeded()
        while newNodeNeeded != None:
            self.AddOneNewNode(newNodeNeeded, uncovered)
            newNodeNeeded, uncovered = self.FindNewNodeNeeded()



    def AddOneNewNode(self, existingNode, uncovered):
        """
        Adds a new node to self that is a child of existingNode and covers one of the minterms from uncovered
        """

        # Add a new internal node to the network
        C = self.InsertBlankInternal()

        # Add C to existingNodes's fan-in
        existingNode.fan_in.append(C)
        C.fan_out.append(existingNode)

        # Update C's level
        C.level = existingNode.level + 1

        # Update Cost of the network
        self.ComputeCost()

        # Update on- & off-sets of C

        '''NAND GATE'''
        if GATE_TYPE == 1:
            # C.on += existingNode.off-set
            C.UpdateOnset(existingNode.off)

            # C.off += one_minterm_from_uncovered ** Performs Covering
            # need to build the minterm expression first
            M = self.PickMinterm(uncovered)
            g = ONE
            for var in M:
                val = M[var]
                if val == 1:
                    g = g & var
                elif val == 0:
                    g = g & ~var
            # Then add this expression to selectedNodes's off-set
            C.UpdateOffset(g)

            # If fan-in limit is reached in existingNode, additional functional updates may be possible
            if len(existingNode.fan_in) == FANIN_BOUND:
                # C.off += existingNode.on * {*sib.on: for all sib in existingNode.fan_in, where sib != C}
                onProd = existingNode.on
                for sib in existingNode.fan_in:
                    if sib != C:
                        onProd = onProd & sib.on
                C.UpdateOffset(onProd)

        '''NOR GATE'''
        if GATE_TYPE == 2:

            # C.off += existingNode.on-set
            C.UpdateOffset(existingNode.on)

            # C.on += one_minterm_from_uncovered ** Performs Covering
            # need to build the minterm expression first
            M = self.PickMinterm(uncovered)
            g = ONE
            for var in M:
                val = M[var]
                if val == 1:
                    g = g & var
                elif val == 0:
                    g = g & ~var
            # Then add this expression to selectedNodes's off-set
            C.UpdateOnset(g)

            # If fan-in limit is reached in existingNode, additional functional updates may be possible
            if len(existingNode.fan_in) == FANIN_BOUND:
                # C.on += existingNode.off * {*sib.off: for all sib in existingNode.fan_in, where sib != C}
                offProd = existingNode.off
                for sib in existingNode.fan_in:
                    if sib != C:
                        offProd = offProd & sib.off
                C.UpdateOnset(offProd)

        # Update Connectible sets
        self.UpdateConnectibleSets()

    def PickMinterm(self, f):
        """
        returns a single minterm from the function f
        """
        for item in list(f.iter_relation()):
            minterm = item[0]
            value = item[1]
            # Check only the the minterms that are true in f
            if value == ONE:
                return minterm

    def FindNewNodeNeeded(self):
        """
        Looks for nodes in the current network that are uncoverable with exisitng nodes
        returns the first node found that is uncoverable
        return node and uncoverable minterms
        if all nodes are coverable then returns (None, None)
        """
        # Look through internal nodes first
        for node in self.internal:
            # Start with all uncovered minterms
            uncoverable = node.uncovered
            # if there are uncovered minterms
            if not(uncoverable.equivalent(ZERO)):
                # walk through connectible nodes to the current node
                for c in node.connectible:
                    # Remove minterms from uncoverable that are covered by c
                    if GATE_TYPE == 1:
                        uncoverable = uncoverable & c.on
                    elif GATE_TYPE == 2:
                        uncoverable = uncoverable & c.off
                # If uncoverable != 0 then there is at least one uncoverable minterm
                # Return this node and the set of uncoverable minterms
                if not(uncoverable.equivalent(ZERO)):
                    strng = ""
                    for item in list(uncoverable.iter_image()):
                        strng += str(item)
                    return node, uncoverable

        # Look through output nodes next
        for node in self.outputs:
            # Start with all uncovered minterms
            uncoverable = node.uncovered
            # if there are uncovered minterms
            if not(uncoverable.equivalent(ZERO)):
                # walk through connectible nodes to the current node
                for c in node.connectible:
                    # Remove minterms from uncoverable that are covered by c
                    if GATE_TYPE == 1:
                        uncoverable = uncoverable & c.on
                    elif GATE_TYPE == 2:
                        uncoverable = uncoverable & c.off
                # If uncoverable != 0 then there is at least one uncoverable minterm
                # Return this node and the set of uncoverable minterms
                if not(uncoverable.equivalent(ZERO)):
                    strng = ""
                    for item in list(uncoverable.iter_image()):
                        strng += str(item)
                    return node, uncoverable

        # If you have not returned yet, then all nodes are coverable
        return None, None


    def ComputeCost(self):
        """
        Computes the cost of the network and returns this value
        Cost = total number of gates in the network
        """
        return len(self.outputs) + len(self.internal)

    def UpdateUncovered(self):
        """
        Updates the uncovered sets for the all the gates in the network
        uncovered sets are updated based on the fan-in to each node
        """

        onMap = {}  # dictionary mapping nodes to BDD representing on-set minterms
        offMap = {} # dictionary mapping nodes to BDD representing off-set minterms

        # for node in self.outputs:
        #     self.CompleteTree(node, onMap, offMap)  ## Not sure the point of this function
        #                                             ## Skipping for now

        # Update the uncovered sets for all of the output nodes
        for node in self.outputs:
            # get the current uncovered set (to see if it changes)
            prevUncovered = node.uncovered
            if GATE_TYPE == 1:
                node.uncovered = node.on  # NAND gates - need to cover on-set minterms
            elif GATE_TYPE == 2:
                node.uncovered = node.off # NOR gates - need to cover off-set minterms

            # the nodes in the fan-in of current node will do covering
            for fi in node.fan_in:
                if GATE_TYPE == 1:
                    node.uncovered = node.uncovered & ~fi.off  # minterm is covered if it is in fi's off-set
                elif GATE_TYPE == 2:
                    node.uncovered = node.uncovered & ~fi.on   # minterm is covered if it is in fi's on-set

            ## For testing purposes
            # if node.uncovered != prevUncovered:
            #     print("update uncovered: ", node, " = ", node.UncoveredFtn2Str())
            ##
        # Update the uncovered sets for all of the internal nodes
        for node in self.internal:
            # get the current uncovered set (to see if it changes)
            prevUncovered = node.uncovered
            if GATE_TYPE == 1:
                node.uncovered = node.on  # NAND gates - need to cover on-set minterms
            elif GATE_TYPE == 2:
                node.uncovered = node.off # NOR gates - need to cover off-set minterms

            # the nodes in the fan-in of current node will do covering
            for fi in node.fan_in:
                if GATE_TYPE == 1:
                    node.uncovered = node.uncovered & ~fi.off  # minterm is covered if it is in fi's off-set
                elif GATE_TYPE == 2:
                    node.uncovered = node.uncovered & ~fi.on   # minterm is covered if it is in fi's on-set

            ## For testing purposes
            # if node.uncovered != prevUncovered:
            #     print("update uncovered: ", node, " = ", node.UncoveredFtn2Str())
            ##

    def UpdateConnectibleSets(self):
        """
        Updates the connectible sets for all the nodes in the network
        """
        ## For testing purposes
        # print("UpdateConnectibleSets")
        ##

        self.UpdateUncovered()

        # Update all the connectible sets for the output nodes
        for node in self.outputs:
            # if there are still minterms to cover
            if not(node.uncovered.equivalent(ZERO)):
                # Find all the nodes in the network that are connectible to this node
                node.connectible = self.FindConnectible(node)
            else:
                # if all the minterms have been covered for this node
                # clear the connectible set of the node
                node.connectible.clear()

            ## For testing purposes
            # print("\t",node, " - connectible set -", end='')
            # for item in node.connectible:
            #     print(" ",item, end='')
            # print()
            ##

        # Update all the connectible sets for the internal nodes
        for node in self.internal:
            # if there are still minterms to cover
            if not(node.uncovered.equivalent(ZERO)):
                # Find all the nodes in the network that are connectible to this node
                node.connectible = self.FindConnectible(node)
            else:
                # if all the minterms have been covered for this node
                # clear the connectible set of the node
                node.connectible.clear()

            ## For testing purposes
            # print("\t",node, " - connectible set -", end='')
            # for item in node.connectible:
            #     print(" ", item, end='')
            # print()
            ##

    """NetworkOrder: A, C = connectible order: primary inputs, fan - in, gate nodes, new
                     B, D = fan - in, primary inputs, gate nodes, new
                     E, F = Randomly Order connectible set
    What is the best way to do this??? Currently using version A, C
    """
    def FindConnectible(self, node):
        """
        Find all the nodes in the network that are connectible to node
        Return the list of nodes.
        """
        CoverNodes = []  # This list will hold all of the nodes that are connectible
        toCover = node.uncovered # this is the list of minterms that still need covering

        # 1st look at all the of the networks inputs
        if len(node.fan_in) < FANIN_BOUND: # Can only add a new fan-in if there is room
            for otherNode in self.inputs:
                if node.StructureCompatible(otherNode):
                    if node.OffSetCompatible(otherNode):
                        if node.OnSetCompatible(otherNode):
                            if GATE_TYPE == 1:
                                totalcover = toCover & ~otherNode.on
                            elif GATE_TYPE == 2:
                                totalcover = toCover & ~otherNode.off
                            if not(totalcover.equivalent(ZERO)):
                                CoverNodes.append(otherNode)

        # 2nd look at the nodes existing fan-in
        for otherNode in node.fan_in:
            if not(otherNode.IsInput()):
                if GATE_TYPE == 1:
                    totalcover = toCover & ~otherNode.on
                elif GATE_TYPE == 2:
                    totalcover = toCover & ~otherNode.off
                if not(totalcover.equivalent(ZERO)):
                    CoverNodes.append(otherNode)

        # 3rd look at all the of the networks internal nodes
        if len(node.fan_in) < FANIN_BOUND: # Can only add a new fan-in if there is room
            for otherNode in self.internal:
                if not(otherNode in node.fan_in):
                    if node.StructureCompatible(otherNode):
                        if node.OffSetCompatible(otherNode):
                            if node.OnSetCompatible(otherNode):
                                if GATE_TYPE == 1:
                                    totalcover = toCover & ~otherNode.on
                                elif GATE_TYPE == 2:
                                    totalcover = toCover & ~otherNode.off
                                if not(totalcover.equivalent(ZERO)):
                                    CoverNodes.append(otherNode)

        # 4th look at all the of the networks output nodes
        if len(node.fan_in) < FANIN_BOUND: # Can only add a new fan-in if there is room
            for otherNode in self.outputs:
                if not(otherNode in node.fan_in):
                    if node.StructureCompatible(otherNode):
                        if node.OffSetCompatible(otherNode):
                            if node.OnSetCompatible(otherNode):
                                if GATE_TYPE == 1:
                                    totalcover = toCover & ~otherNode.on
                                elif GATE_TYPE == 2:
                                    totalcover = toCover & ~otherNode.off
                                if not(totalcover.equivalent(ZERO)):
                                    CoverNodes.append(otherNode)

        return CoverNodes

    def FindConnectibleSet(self, selectedNode, selectedMinterm):
        """
        Returns the connectible set from selectedNode that can be used to cover selectedMinterm
        NO SYMMETRY REMOVAL DONE - remove all but one of the input nodes from a symmetric set
        """
        ## For testing purposes
        # print("Find Connectible Set")
        ##

        keep = []
        for node in selectedNode.connectible:
            # A nand gate node can cover if it does not contain the minterm in it's on-set
            # A nor gate node can cover if it does not contain the minterm in it's off-set
            if ((GATE_TYPE == 1 and node.on.restrict(selectedMinterm).equivalent(ZERO)) or
                (GATE_TYPE == 2 and node.off.restrict(selectedMinterm).equivalent(ZERO))):
                keep.append(node)
                ## For testing purposes
                # print("\t",node)
                ##

        # Add a blank node to the connection set as long as selectedNode has an open fan-in for it
        if len(selectedNode.fan_in) < FANIN_BOUND:
            # Create internal node
            n = Node()
            n.idx = self.vars
            self.vars += 1
            n.level = 0
            if GATE_TYPE == 1:
                n.logic_op = 'D'
            elif GATE_TYPE == 2:
                n.logic_op = 'R'

            ## For testing purposes
            # print("\t",n)
            ##

            keep.append(n)

        return keep



    def PrintNetwork(self):
        """
        Prints a text base version of the network
        """
        # print network index
        print("vvvvvvvvvv",self.idx,"vvvvvvvvvv")
        # print info about outputs
        for node in self.outputs:
            # # Print fan-out to node
            # print("(",end="")
            # for fo in node.fan_out:
            #     print(fo,end=", ")
            # print(") =",end="")

            # Print node index
            print(node,"= (",end="")

            # Print fan-in to node
            for fi in node.fan_in:
                if fi != node.fan_in[-1]:
                    print(fi,end=", ")
                else:
                    print(fi,end="")

            # Print function interval
            print(") =", node.FtnToStr())

        # print info about internal nodes
        for node in self.internal:
            # Print fan-out to node
            # print("(",end="")
            # for fo in node.fan_out:
            #     print(fo,end=", ")
            # print(") =",end="")

            # Print node index
            print(node," = (",end="")

            # Print fan-in to node
            for fi in node.fan_in:
                if fi != node.fan_in[-1]:
                    print(fi,end=", ")
                else:
                    print(fi,end="")

            # Print function interval
            print(") =", node.FtnToStr())

        # print info about inputs
        for node in self.inputs:
            # Print fan-out to node
            # print("(",end="")
            # for fo in node.fan_out:
            #     print(fo,end=", ")
            # print(") =",end="")

            # Print node index
            print(node," = ", node.FtnToStr())

        print("^^^^^^^^^^",self.idx,"^^^^^^^^^^")
        print()

    def WriteNetwork(self, outFile):
        """
        Writes a text base version of the network to a file
        """
        # write info about outputs
        for node in self.outputs:

            outFile.write("O"+str(node.idx)+"=")

            if node.logic_op == 'D':
                outFile.write("NAND(")
            elif node.logic_op == 'R':
                outFile.write("NOR(")

            for fi in node.fan_in:
                if fi in self.outputs:
                    outFile.write("O"+str(fi.idx))
                elif fi in self.inputs:
                    outFile.write("x"+str(fi.idx))
                else:
                    outFile.write("I"+str(fi.idx))

                if fi != node.fan_in[-1]:
                    outFile.write(", ")
                else:
                    outFile.write(")::")

        # print info about internal nodes
        for node in self.internal:
            outFile.write("I"+str(node.idx)+"=")

            if node.logic_op == 'D':
                outFile.write("NAND(")
            elif node.logic_op == 'R':
                outFile.write("NOR(")

            for fi in node.fan_in:
                if fi in self.outputs:
                    outFile.write("O"+str(fi.idx))
                elif fi in self.inputs:
                    outFile.write("x"+str(fi.idx))
                else:
                    outFile.write("I"+str(fi.idx))

                if fi != node.fan_in[-1]:
                    outFile.write(", ")
                else:
                    outFile.write(")::")

        outFile.write("\n")

    def AllCovered(self):
        """
        Checks all nodes in the network to see if all coverable  minterms are covered.
        returns True if all minterms are covered
        return False if at least one minterm remains uncovered.
        """
        for node in self.outputs:
            if not(node.uncovered.equivalent(ZERO)):
                return False
        for node in self.internal:
            if not(node.uncovered.equivalent(ZERO)):
                return False
        return True

    def SelectMintermForCovering(self):
        """
        Selects an uncovered minterm for covering
        returns a tuple (minterm, node)
        """
        # Can change ordering for selection.
        # This version selects nodes & minterms in the following order
        # 1st - Find the node with (a) smallest fan-in (b) smallest connectible set (c) Largest index
        # 2nd - Find the minterm with (a) fewest coverings possible (b) smallest index)
        ## For Testing
        # print("Select Node:")
        ##
        selectedNode = None
        selectedMinterm = None
        for node in self.internal:
            if not(node.uncovered.equivalent(ZERO)):
                if (
                    (selectedNode == None) or
                    (len(node.fan_in) < len(selectedNode.fan_in)) or
                    (len(node.fan_in) == len(selectedNode.fan_in) and len(node.connectible) < len(selectedNode.connectible))
                   ):
                    selectedNode = node
                    toCover = node.uncovered
        for node in self.outputs:
            if not(node.uncovered.equivalent(ZERO)):
                if (
                    (selectedNode == None) or
                    (len(node.fan_in) < len(selectedNode.fan_in)) or
                    (len(node.fan_in) == len(selectedNode.fan_in) and len(node.connectible) < len(selectedNode.connectible))
                   ):
                    selectedNode = node
                    toCover = node.uncovered
        ## For Testing
        # print("  Selected Node = ", selectedNode)
        # print("  To Cover = ", selectedNode.UncoveredFtn2Str())
        # print("  Select Minterm")
        ##

        #Count the number of minterms that need to be covered from this node
        numMinterms = toCover.satisfy_count()

        # If there is only one minterm in toCover then pick this one
        if numMinterms == 1:
            selectedMinterm = toCover.satisfy_one()
        else:
            # Select the best minterm for covering
            # Find the minterm with (a) fewest coverings possible (b) smallest index)
            minCovering = INF
            #Iterate over all possible minterms - THIS IS NOT EFFICIENT
            for item in list(toCover.iter_relation()):
                minterm = item[0]
                value = item[1]
                # Check only the the minterms that are true in toCover
                if value == ONE:
                    ct = 0
                    # Count the number of connectible nodes that can cover this minterm
                    for node in selectedNode.connectible:
                        if GATE_TYPE == 1:
                            # A nand gate node can cover if it does not contain the minterm in it's on-set
                            if node.on.restrict(minterm).equivalent(ZERO):
                                ct += 1
                        elif GATE_TYPE == 2:
                            # A nor gate node can cover if it does not contain the minterm in it's off-set
                            if node.off.restrict(minterm).equivalent(ZERO):
                                ct += 1
                    ## For testing purposes
                    # print("\t", minterm, ct)
                    ##
                    if ct < minCovering:
                        # If this this the fewest coverings so far, then store it as the selected minterm
                        selectedMinterm = minterm
                        minCovering = ct

        ## For Testing
        # print("  Selected MInterm = ", selectedMinterm)
        ##

        return selectedNode, selectedMinterm
    '''
    PerformCovering(Network, N, C, m) { 
        if (C is an input node) {
            N.fan_in ← N.fan_in ∪ {C};
            C.fan_out ← C.fan_out ∪ {N};
        } else if (C ∈ N.fan_in) {
            C.off ← C.off ∨ m;
        } else if (C is an existing gate node) {
            N.fan_in ← N.fan_in ∪ {C}; 
            C.fan_out ← C.fan_out ∪ {N}; 
            C.off ← C.off ∨ m;
        } else if (C is a new gate) {
            N.fan_in ← N.fan_in ∪ {C}; 
            C.fan_out ← C.fan_out ∪ {N}; 
            C.off ← m;
        }
        Return Network;
    }
    '''
    def PerformCovering(self, selectedNode, coveringNode, selectedMinterm):
        """
        Creates a new network that uses selectedNode to cover selectedMinterm in coveringNode
        Functional Implication are propogated as the on & off-sets of selctedNode and coveringNode are
        changed to peform the covering
        Returns the new network
        """
        ## For testing purposes
        # print("Perform Covering -",selectedNode, coveringNode, selectedMinterm)
        ##

        # Copy Network
        newNetwork, mapping = self.CopyNetwork()


        #Get the new nodes for selectedNode and coveringNode in the newNetwork
        cn = mapping[coveringNode]
        # Might need to create a new node in newNetwork for sn if it's new
        if (not (selectedNode in self.inputs) and
            not (selectedNode in self.outputs) and
            not (selectedNode in self.internal)):
            # Create internal node
            sn = Node()
            sn.idx = newNetwork.vars
            newNetwork.vars += 1
            sn.level = 0
            if GATE_TYPE == 1:
                sn.logic_op = 'D'
            elif GATE_TYPE == 2:
                sn.logic_op = 'R'
        # Otherwise you can get it from the mapping
        else:
            sn = mapping[selectedNode]


        # Add selectedNode to coverNode's fan-in if it is not already there
        if not(sn in cn.fan_in):
            cn.fan_in.append(sn)
            sn.fan_out.append(cn)

            # Update sn's level
            if cn.level + 1 > sn.level:
                sn.level = cn.level + 1

            # Add sn to internal nodes if it is not already in the network
            if (not(sn in newNetwork.inputs) and
                not(sn in newNetwork.outputs) and
                not(sn in newNetwork.internal)):
                newNetwork.internal.append(sn)

        # Update Cost of the network
        newNetwork.ComputeCost()

        '''NAND GATE'''
        if GATE_TYPE == 1:
            # Update on- & off-sets of sn and cn
            # Functional Implications are performed during these updates

            # cn.on-set += sn.off-set
            status = cn.UpdateOnset(sn.off)
            if status == False:  # There was an error and this network will not work
                return None

            # sn.on += cn.off-set
            status = sn.UpdateOnset(cn.off)
            if status == False:  # There was an error and this network will not work
                return None

            # sn.off += selectedMinterm ** Performs Covering
            # need to building the minterm expression first
            g = ONE
            for var in selectedMinterm:
                val = selectedMinterm[var]
                if val == 1:
                    g = g & var
                elif val == 0:
                    g = g & ~var
            # Then add this expression to selectedNodes's off-set
            status = sn.UpdateOffset(g)
            if status == False:  # There was an error and this network will not work
                return None

            # If fan-in limit is reached in cn, additional functional updates may be possible
            if len(cn.fan_in) == FANIN_BOUND:
                # cn.off += {*I.on : For all I in cn.fan_in}
                # For 2 inputs: cn.off = sn.on * sibling.on
                onProd = ONE
                for node in cn.fan_in:
                    onProd = onProd & node.on
                status = cn.UpdateOffset(onProd)
                if status == False:  # There was an error and this network will not work
                    return None

                # sibling.off += cn.on * {*I.on: for all I in cn.fan_in, where I != sibling}
                for sib in cn.fan_in:
                    onProd = cn.on
                    for node in cn.fan_in:
                        if node != sib:
                            onProd = onProd & node.on
                    status = sib.UpdateOffset(onProd)
                    if status == False:  # There was an error and this network will not work
                        return None

        '''NOR GATE'''
        if GATE_TYPE == 2:
            # Update on- & off-sets of sn and cn
            # Functional Implications are performed during these updates

            # cn.off-set += sn.on-set
            status = cn.UpdateOffset(sn.on)
            if status == False:  # There was an error and this network will not work
                return None

            # sn.off += cn.on-set
            status = sn.UpdateOffset(cn.on)
            if status == False:  # There was an error and this network will not work
                return None

            # sn.on += selectedMinterm ** Performs Covering
            # need to building the minterm expression first
            g = ONE
            for var in selectedMinterm:
                val = selectedMinterm[var]
                if val == 1:
                    g = g & var
                elif val == 0:
                    g = g & ~var
            # Then add this expression to selectedNodes's on-set
            status = sn.UpdateOnset(g)
            if status == False:  # There was an error and this network will not work
                return None

            # If fan-in limit is reached in cn, additional functional updates may be possible
            if len(cn.fan_in) == FANIN_BOUND:
                # cn.on += {*I.off : For all I in cn.fan_in}
                # For 2 inputs: cn.on = sn.off * sibling.off
                offProd = ONE
                for node in cn.fan_in:
                    offProd = offProd & node.off
                status = cn.UpdateOnset(offProd)
                if status == False:  # There was an error and this network will not work
                    return None

                # sibling.on += cn.off * {*I.off: for all I in cn.fan_in, where I != sibling}
                for sib in cn.fan_in:
                    offProd = cn.off
                    for node in cn.fan_in:
                        if node != sib:
                            offProd = offProd & node.off
                    status = sib.UpdateOnset(offProd)
                    if status == False:  # There was an error and this network will not work
                        return None


        # Simple Bridge Check - Skip for now

        # Extended Bridge Check - Skip for now

        # Update Connectible sets
        newNetwork.UpdateConnectibleSets()

        # Structural Implication - add new gates that cannot be covered by exisitng nodes now
        # only works for nand only or nor only networks
        if GATE_TYPE == 1 or GATE_TYPE == 2:
            newNetwork.AddNewNodes()


        newNetwork.cost = newNetwork.ComputeCost()

        return newNetwork





if __name__ == "__main__":

    # Create input & output BDDS for network
    a = exprvar('a')
    b = exprvar('b')
    o = (~a & ~b) | (a & b)
    # Create network
    nn = Network()

    # Initialize network
    nn.Initialization([a,b], o)
    nn.PrintNetwork()

    # # Update Connectible Set
    # nn.UpdateConnectibleSets()
    #
    # # Find minterm to cover
    # coveringNode, coverMinterm = nn.SelectMintermForCovering()
    #
    # # Find connectible set of covering minterm
    # set = nn.FindConnectibleSet(coveringNode, coverMinterm)
    #
    # nn2 = nn.PerformCovering(set[0], coveringNode, coverMinterm)
    #
    # nn.PrintNetwork()
    # nn2.PrintNetwork()

    # # Find minterm to cover
    # coveringNode, coverMinterm = nn2.SelectMintermForCovering()
    #
    # # Find connectibel set of covering minterm
    # set = nn2.FindConnectibleSet(coveringNode, coverMinterm)
    #
    # nn3 = nn2.PerformCovering(set[0], coveringNode, coverMinterm)
    #
    # nn.PrintNetwork()
    # nn2.PrintNetwork()
    # nn3.PrintNetwork()
    #
    # # Find minterm to cover
    # coveringNode, coverMinterm = nn3.SelectMintermForCovering()
    #
    # # Find connectibel set of covering minterm
    # set = nn3.FindConnectibleSet(coveringNode, coverMinterm)
    #
    # nn4 = nn3.PerformCovering(set[0], coveringNode, coverMinterm)
    #
    # nn.PrintNetwork()
    # nn2.PrintNetwork()
    # nn3.PrintNetwork()
    # nn4.PrintNetwork()
    #
    # # Find minterm to cover
    # coveringNode, coverMinterm = nn4.SelectMintermForCovering()
    #
    # # Find connectibel set of covering minterm
    # set = nn4.FindConnectibleSet(coveringNode, coverMinterm)
    #
    # nn5 = nn4.PerformCovering(set[0], coveringNode, coverMinterm)
    #
    # nn.PrintNetwork()
    # nn2.PrintNetwork()
    # nn3.PrintNetwork()
    # nn4.PrintNetwork()
    # nn5.PrintNetwork()
    #
    # # Find minterm to cover
    # coveringNode, coverMinterm = nn5.SelectMintermForCovering()
    #
    # # Find connectibel set of covering minterm
    # set = nn5.FindConnectibleSet(coveringNode, coverMinterm)
    #
    # nn6 = nn5.PerformCovering(set[0], coveringNode, coverMinterm)
    #
    # nn6.PrintNetwork()
    #
    # # Find minterm to cover
    # coveringNode, coverMinterm = nn6.SelectMintermForCovering()
    #
    # # Find connectibel set of covering minterm
    # set = nn6.FindConnectibleSet(coveringNode, coverMinterm)
    #
    # nn7 = nn6.PerformCovering(set[0], coveringNode, coverMinterm)
    #
    # nn7.PrintNetwork()
    #
    # # Find minterm to cover
    # coveringNode, coverMinterm = nn7.SelectMintermForCovering()
    #
    # # Find connectibel set of covering minterm
    # set = nn7.FindConnectibleSet(coveringNode, coverMinterm)
    #
    # nn8 = nn7.PerformCovering(set[0], coveringNode, coverMinterm)
    #
    # nn8.PrintNetwork()
    #
    # # Find minterm to cover
    # coveringNode, coverMinterm = nn8.SelectMintermForCovering()
    #
    # # Find connectibel set of covering minterm
    # set = nn8.FindConnectibleSet(coveringNode, coverMinterm)
    #
    # nn9 = nn8.PerformCovering(set[0], coveringNode, coverMinterm)
    #
    # nn9.PrintNetwork()
    #
    # print(nn9.AllCovered())


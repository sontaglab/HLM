from pyeda.inter import *   # Needed for BDDs

'''
Author: Beth Ernst - Macalester College - eernst@macalester.edu
Date: 8/5/19

This is a data structure file.  Intended to be used and run from main.py

The node substructure will maintain all the data needed to describe the node. This includes the global function
associated with the node (stored in terms of the on- and off-sets of the Boolean function), the type of node (whether
the node is an input node or gate node), and the local function associated with the gate nodes (NAND). The node
structure must also maintain three sets which describe the relationship of this node to others in the network. These
sets are the fan-in, fan-out, and connectible set for the node. Finally, the node structure will keep a Boolean function
that describes the uncovered on-set minterms at this node. This uncovered function will help to determine the work that
remains to be done at this node.


Pseudocode for the node data structure:

Structure Node {
Integer idx    //Unique index
Integer level  //distance from inputs
BoolFunc on    //Global Function
BoolFunc off   //Global Function
BoolFunc uncovered  //Uncovered portion of the on-set
NodeType input or gate
Set fan_in     //Fan-in set (children of this node)
Set fan_out    //Fan-out set (parents of this node)
Set connectible  //Set of nodes in the network that are connectible to current node
char LogicOp   //Logical Operator of Gate
               //'D' -> NAND, 'R' -> NOR

//Do I need these?
Set alreadyConnected  // ????
int Label             // ????
}
'''

# Global Variables - Change according to problem description
INF = 10000     # "infinity"
FANIN_BOUND = 2 # Limits the fan-in size to each gate ##*** CHANGE FAN-IN LIMIT HERE  ***##
ZERO = expr(0)
ONE = expr(1)
GATE_TYPE = 2   # 1: NAND, 2: NOR, 3: AND/OR/NOT, 5: NAND/NOR  ##*** CHANGE GATE TYPE HERE  ***##


class Node:
    """ Node Class for representing nodes in the network    """

    def __init__(self):
        self.idx = 0          # unique index
        self.level = 0        # distance from output; all inputs have level -1
        self.on = ZERO        # BDD representing the on-set of the Boolean Function
        self.off = ZERO       # BDD representing the off-set of the Boolean Function
        self.uncovered = ZERO # BDD representing the minterms not covered in the Boolean Function
        self.fan_in = []      # List of nodes in the fan-in set of the node (children)
        self.fan_out = []     # List of nodes in the fan-out set of the node (parents)
        self.connectible = [] # List of nodes in the network that can be connected to the current node
        self.logic_op = 'I'    # Logic operator of the current node
                              # 'I' -> Input, 'D' -> NAND, 'R' -> NOR

    def IsInput(self):
        # All input nodes have level -1
        return self.level == -1

    def Ancestor(self, node):
        """
        Determines if node appears on any path from self to a primary output
        returns True if node is an ancestor of self
        """
        # If node appears in the fan-out of self then node is an ancestor of self
        if node in self.fan_out:
            return True
        # Now check the parents of self looking for node
        for parent in self.fan_out:
            if parent.Ancestor(node):
                return True
        return False

    def StructureCompatible(self, node):
        """
        Determines if node is compatible as a fan-in to self based on structure constraints
        """
        # Cannot connect a node to itself
        if self == node:
            return False
        # If the node is already in the fan_in of self it's strutural ok
        # Need to check more situations if it is not in the fan-in
        elif not(node in self.fan_in):
            # Cannot connect a new node if the fan-in limit has been reached
            if len(self.fan_in) >= FANIN_BOUND:
                return False
            # Cannot connect a new node if it is an ancestor of the node
            # ie node cannot appear  on any path from self to a primary output.
            elif self.Ancestor(node):
                return False

        # Currently no fan-out restriction

        return True

    def OffSetCompatible(self, node):
        """
        Determines if node is a compatible fan-in to self based on the off-sets of the nodes
        """
        if GATE_TYPE == 1:  #NAND Gates
            # self & node cannot have any matching off-set minterms
            return ZERO.equivalent(self.off & node.off)
        elif GATE_TYPE == 2:  #NOR Gates
            # As long as there are 2 or more open spots off-sets are compatible
            if len(self.fan_in) + 1 < FANIN_BOUND:
                return True
            else:
                # If the fan-in is full then at least one fan-in has to be able to cover the off-set minterms of self
                result = self.off & node.off
                for otherNode in self.fan_in:
                    result = result & otherNode.off
                return ZERO.equivalent(result)

    def OnSetCompatible(self, node):
        """
        Determines if node is a compatible fan-in to self based on the on-sets of the nodes
        """
        if GATE_TYPE == 1:  #NAND Gates
            # As long as there are 2 or more open spots on-sets are compatible
            if len(self.fan_in) + 1 < FANIN_BOUND:
                return True
            else:
            # If the fan-in is full then at least one fan-in has to be able to cover the on-set minterms of self
                result = self.on & node.on
                for otherNode in self.fan_in:
                    result = result & otherNode.on
                return ZERO.equivalent(result)
        elif GATE_TYPE == 2:  #NOR GATES
            # self & node cannot have any matching on-set minterms
            return ZERO.equivalent(self.on & node.on)



    def __str__(self):
        return str(self.idx) + "(" + self.logic_op + ")"


    def UncoveredFtn2Str(self):
        """
        returns a string of 1s, 0s, and *s representing the truth table for the uncovered bdd of self
        starts at the 00..0 row of truth table through the 11..1 of truth table
        a 0 occurs if the minterm is a 0 or already covered by one of the node's inputs
        a 1 occurs if the minterm still needs to be covered
        """

        strng=""
        for item in list(self.uncovered.iter_image()):
            strng += str(item)

        return strng

    def FtnToStr(self):
        """
        returns a string of 1s 0s ?s and -s representing the truth table for the function interval of self
        starts at 00..0 row of truth table through 11..1 of truth table
        a 0 occurs if the minterm is in the off-set
        a 1 occurs if the minterm is in the on-set
        a ? occurs if the minterm is in the on- & off- set (this should NOT happen)
        a - occurs if the minterm is in either the on-set nor the off-set (this is ok)
        """

        if len(self.on.inputs) > len(self.off.inputs):
            inputList = list(self.on.inputs)
        else:
            inputList = list(self.off.inputs)
        str = ""
        for minterm in list(iter_points(inputList)):
            onVal = self.on.restrict(minterm)
            offVal = self.off.restrict(minterm)
            if onVal.equivalent(ONE):
                if offVal.equivalent(ZERO):
                    str += "1"
                else:
                    str += "?"
            elif onVal.equivalent(ZERO):
                if offVal.equivalent(ONE):
                    str += "0"
                else:
                    str += "-"
        return str




    def UpdateOnset(self, f):
        """
        Updates the on-set of self
        The changes made are dependent on the type of gate that self is
        Checks to make sure this is a vaild update
        If the update is valid, propagates this change through the network
        Returns True or False based on the validity of the update
        """

        """ 
        INPUT Node
        No updates possible
        """
        if self.logic_op == 'I':
            return True

        """
        NAND GATE
        self.on = self.on | f
        """
        if self.logic_op == 'D':

            ## For testing purposes
            # print(self)
            # print("\ton - ",end='')
            # for item in list(self.on.iter_image()):
            #     if(item.equivalent(ONE)):
            #         print(item,end='')
            #     else:
            #         print("-",end='')
            # print()
            # print("\tf - ", end='')
            # for item in list(f.iter_image()):
            #     if(item.equivalent(ONE)):
            #         print(item,end='')
            #     else:
            #         print("-",end='')
            # print()
            ##

            oldON = self.on
            self.on = self.on | f

            ## For testing purposes
            # print("\tresult - ",end='')
            # for item in list(self.on.iter_image()):
            #     if(item.equivalent(ONE)):
            #         print(item,end='')
            #     else:
            #         print("-",end='')
            #
            # print()
            ##

            ''' Check that the new on-set is valid:
                1. the on- and off-sets cannot overlap
                2. the on-set should never contain all the minterms
                3. the off-set should never contain all the minterms
            '''
            if not((self.on & self.off).equivalent(ZERO)) or (self.on).equivalent(ONE) or (self.off).equivalent(ONE):
                ## For testing purposes
                #  print("ERROR in updating on-set of ", self)
                ##
                return False

            ''' If a change has been made to the on-set then the change should be propogated to the off-sets of
                the parents and children
            '''
            if not(oldON.equivalent(self.on)):
                '''Update the off-sets of the parents of self'''
                for fo in self.fan_out:
                    '''Off-set can be updated if the fan-in bound has been reached'''
                    if len(fo.fan_in) == FANIN_BOUND:
                        ## For testing purposes
                        #  print(fo, ":", len(fo.fan_in), "==", FANIN_BOUND)
                        ##
                        '''fo.off += {*I.on : For all I in cn.fan_in}'''
                        onProd = ONE
                        for node in fo.fan_in:
                            onProd = onProd & node.on
                        ## For testing purposes
                        #  print(self,"->")
                        ##
                        if not(fo.UpdateOffset(onProd)):
                            return False

                        '''sibling.off += fo.on * {*I.on: for all I in fo.fan_in, where I != sibling}'''
                        for sib in fo.fan_in:
                            if sib != self:
                                onProd = fo.on
                                for node in fo.fan_in:
                                    if node != sib:
                                        onProd = onProd & node.on
                                ## For testing purposes
                                #  print(self, "->")
                                ##
                                if not(sib.UpdateOffset(onProd)):
                                    return False

                '''Off-set of the children can be updated if the fan-in bound has been reached'''
                if len(self.fan_in) == FANIN_BOUND:
                    for fi in self.fan_in:
                        '''fi.off += self.on * {*I.on: for all I in self.fan_in, where I != fi}'''
                        onProd = self.on
                        for node in self.fan_in:
                            if node != fi:
                                onProd = onProd & node.on
                        ## For testing purposes
                        # print(self,"->")
                        ##
                        if not(fi.UpdateOffset(onProd)):
                            return False

            return True

        """
        NOR GATE
        self.on = self.on | f
        """
        if self.logic_op == 'R':

            oldON = self.on
            self.on = self.on | f


            ''' Check that the new on-set is valid:
                1. the on- and off-sets cannot overlap
                2. the on-set should never contain all the minterms
                3. the off-set should never contain all the minterms
            '''
            if not ((self.on & self.off).equivalent(ZERO)) or (self.on).equivalent(ONE) or (self.off).equivalent(ONE):
                return False

            ''' If a change has been made to the on-set then the change should be propagated to the off-sets of
                the parents and children
            '''
            if not (oldON.equivalent(self.on)):
                '''Update the off-sets of the parents of self'''
                for fo in self.fan_out:
                    '''fo.off += self.on'''
                    if not (fo.UpdateOffset(self.on)):
                        return False

                '''Update the off-sets of the children of self'''
                for fi in self.fan_in:
                    '''fi.off += self.on'''
                    if not (fi.UpdateOffset(self.on)):
                        return False

            return True



    def UpdateOffset(self, f):
        """
        Updates the off set of self.
        Checks to make sure this is a valid update
        If the update is valide, propagates this change through the network
        Returns True or False based on validity of update
        """

        """ 
        INPUT Node
        No updates possible
        """
        if self.logic_op == 'I':
            return True

        """
        NAND GATE
        self.off = self.off | f
        """
        if self.logic_op == 'D':
            ## For testing purposes
            # print(self)
            # print("\toff - ",end='')
            # for item in list(self.off.iter_image()):
            #     if(item.equivalent(ONE)):
            #         print(~item,end='')
            #     else:
            #         print("-",end='')
            # print()
            # print("\tf - ",end='')
            # for item in list(f.iter_image()):
            #     if(item.equivalent(ONE)):
            #         print(~item,end='')
            #     else:
            #         print("-",end='')
            # print()
            ##

            oldOFF = self.off
            self.off = self.off | f

            ## For testing purposes
            # print("\tresult - ",end='')
            # for item in list(self.off.iter_image()):
            #     if(item.equivalent(ONE)):
            #         print(~item,end='')
            #     else:
            #         print("-",end='')
            # print()
            ##

            ''' Check that the new off-set is valid:
                1. the on- and off-sets cannot overlap
                2. the on-set should never contain all the minterms
                3. the off-set should never contain all the minterms
            '''
            if not((self.on & self.off).equivalent(ZERO)) or (self.on).equivalent(ONE) or (self.off).equivalent(ONE):
                ## For testing purposes
                #  print("ERROR in updating off-set of", self)
                ##
                return False

            ''' If a change has been made to the off-set then the change should be propagated to the on-sets of
                the parents and children
            '''
            if not(oldOFF.equivalent(self.off)):
                '''Update the on-sets of the parents of self'''
                for fo in self.fan_out:
                    '''fo.on += self.off'''
                    ## For testing purposes
                    # print(self, "->")
                    ##
                    if not (fo.UpdateOnset(self.off)):
                        return False

                '''Update the on-sets of the children of self'''
                for fi in self.fan_in:
                    '''fi.on += self.off'''
                    ## For testing purposes
                    #  print(self, "->")
                    ##
                    if not (fi.UpdateOnset(self.off)):
                        return False

            return True

        """
        NOR GATE
        self.on = self.on | f
        """
        if self.logic_op == 'R':

            oldOFF = self.off
            self.off = self.off | f

            ''' Check that the new off-set is valid:
                            1. the on- and off-sets cannot overlap
                            2. the on-set should never contain all the minterms
                            3. the off-set should never contain all the minterms
            '''
            if not ((self.on & self.off).equivalent(ZERO)) or (self.on).equivalent(ONE) or (self.off).equivalent(ONE):
                return False

            ''' If a change has been made to the off-set then the change should be propagated to the on-sets of
                the parents and children
            '''
            if not(oldOFF.equivalent(self.off)):
                '''Update the on-sets of the parents of self'''
                for fo in self.fan_out:
                    '''On-set can be updated if the fan-in bound has been reached'''
                    if len(fo.fan_in) == FANIN_BOUND:
                        '''fo.on += {*I.off : For all I in cn.fan_in}'''
                        offProd = ONE
                        for node in fo.fan_in:
                            offProd = offProd & node.off

                        if not(fo.UpdateOnset(offProd)):
                            return False

                        '''sibling.on += fo.off * {*I.off: for all I in fo.fan_in, where I != sibling}'''
                        for sib in fo.fan_in:
                            if sib != self:
                                offProd = fo.off
                                for node in fo.fan_in:
                                    if node != sib:
                                        offProd = offProd & node.off
                                if not(sib.UpdateOnset(offProd)):
                                    return False

                '''On-set of the children can be updated if the fan-in bound has been reached'''
                if len(self.fan_in) == FANIN_BOUND:
                    for fi in self.fan_in:
                        '''fi.on += self.off * {*I.off: for all I in self.fan_in, where I != fi}'''
                        offProd = self.off
                        for node in self.fan_in:
                            if node != fi:
                                offProd = offProd & node.off
                        if not(fi.UpdateOnset(offProd)):
                            return False

            return True

    def Pruning(self, c, m):
        """
        Perform pruning of the search space based on overlapped covering.
        Once a node C is used to cover the minterm m, if C is a fan-in of Node, the minterm m can be added to the on-set
        of C ensuring that C will not be able cover m for the rest of the partial networks created from this call.
        """
        '''
        NAND Gate -  if (C ∈ Node.fan_in) C.on ← C.on ∨ m   
        NOR Gate  -  if (C ∈ Node.fan_in) C.off ← C.off v m
        '''

        if c in self.fan_in:
            # need to build the minterm expression first
            g = ONE
            for var in m:
                val = m[var]
                if val == 1:
                    g = g & var
                elif val == 0:
                    g = g & ~var
            if GATE_TYPE == 1:  #NAND
                # Then add this expression to c's on-set
                c.UpdateOnset(g)
            if GATE_TYPE == 2:  # NOR
                # Then ad                                                                                                                                d this expression to c's off-set
                c.UpdateOffset(g)


if __name__ == "__main__":
    c = exprvar('c')
    b = exprvar('b')
    a = exprvar('a')

    # Make input node a
    aN = Node()
    aN.on = a
    aN.off = ~a
    aN.level = -1
    aN.idx = 0

    # Make input node b
    bN = Node()
    bN.on = b
    bN.off = ~b
    bN.level = -1
    bN.idx = 1

    # Make input node c
    cN = Node()
    cN.on = c
    cN.off = ~c
    cN.level = -1
    cN.idx = 2

    # Make ouptut node n
    n = Node()
    n.on = (~a & ~c) | (~a & b) | (a & ~b)
    n.off = (~a & ~b & c) | (a & b)
    n.idx = 3
    n.uncovered = n.on
    n.logic_op = 'D'


    # Make internal node m2
    m2 = Node()
    m2.on = (~a & ~b & c) | (a & b)
    m2.off = ~a & ~c
    m2.idx = 4
    m2.logic_op = 'D'

    # Make internal node m3
    m3 = Node()
    m3.on = ~c
    m3.off = ~a & ~b & c
    m3.idx = 5
    m3.logic_op = 'D'

    # Make internal node m4
    m4 = Node()
    m4.on = ~a
    m4.off = a & b & ~c
    m4.idx = 6
    m4.logic_op = 'D'

    # Make internal node m5
    m5 = Node()
    m5.on = (~a & ~b & c) | (a & b)
    m5.off = ~a & b & c
    m5.idx = 7
    m5.logic_op = 'D'

    print('-------------------------')
    print(aN)
    print(aN.FtnToStr())
    print()
    print(bN)
    print(bN.FtnToStr())
    print()
    print(cN)
    print(cN.FtnToStr())
    print()
    print(n)
    print(n.FtnToStr())
    print()
    print(m2)
    print(m2.FtnToStr())
    print(m3)
    print(m3.FtnToStr())
    print(m4)
    print(m4.FtnToStr())
    print(m5)
    print(m5.FtnToStr())
    print('-------------------------')

    # Add m2 as a child of n
    n.fan_in.append(m2)
    m2.fan_out.append(n)

    # Add m5 as a child of n
    n.fan_in.append(m5)
    m5.fan_out.append(n)

    # Add m3 as a child of m2
    m2.fan_in.append(m3)
    m3.fan_out.append(m2)

    # Add m4 as a child of m2
    m2.fan_in.append(m4)
    m4.fan_out.append(m2)

    # Add c as a child of m3
    m3.fan_in.append(cN)
    cN.fan_out.append(m3)

    # Add a as a child of m4
    m4.fan_in.append(aN)
    aN.fan_out.append(m4)


    print('-------------------------')
    print(aN)
    print(aN.FtnToStr())
    print()
    print(bN)
    print(bN.FtnToStr())
    print()
    print(cN)
    print(cN.FtnToStr())
    print()
    print(n)
    print(n.FtnToStr())
    print()
    print(m2)
    print(m2.FtnToStr())
    print(m3)
    print(m3.FtnToStr())
    print(m4)
    print(m4.FtnToStr())
    print(m5)
    print(m5.FtnToStr())
    print('-------------------------')

    # Use b to cover a'b'c in m5

    # Add b as a child of m5
    m5.fan_in.append(bN)
    bN.fan_out.append(m5)

    print("update on set of b")
    bN.UpdateOnset(m5.off)
    print("update on set of m5")
    m5.UpdateOnset(bN.off)
    print("update off set of b")
    bN.UpdateOffset(~a & ~b & c)

    print('-------------------------')
    print(aN)
    print(aN.FtnToStr())
    print()
    print(bN)
    print(bN.FtnToStr())
    print()
    print(cN)
    print(cN.FtnToStr())
    print()
    print(n)
    print(n.FtnToStr())
    print()
    print(m2)
    print(m2.FtnToStr())
    print(m3)
    print(m3.FtnToStr())
    print(m4)
    print(m4.FtnToStr())
    print(m5)
    print(m5.FtnToStr())
    print('-------------------------')

    # # Test on & off set compatibility
    # print(n.OnSetCompatible(m))
    # print(n.OffSetCompatible(m))
    #
    # print(n.OnSetCompatible(aN))
    # print(n.OffSetCompatible(aN))
    #
    # print(n.OnSetCompatible(bN))
    # print(n.OffSetCompatible(bN))
    #
    # print("***********************")
    # # Test structure compatibility
    #
    # print(n.StructureCompatible(m))
    # print(n.StructureCompatible(aN))
    # print(n.StructureCompatible(bN))
    # print(n.StructureCompatible(n))
    # print("--------------------------")
    # print(m.StructureCompatible(m))
    # print(m.StructureCompatible(aN))
    # print(m.StructureCompatible(bN))
    # print(m.StructureCompatible(n))


# -*- coding: utf-8 -*-
from pyeda.inter import *   # Needed for BDDs
import time
from Network import *

"""
Author: Beth Ernst - Macalester College - eernst@macalester.edu
Date: 8/5/19

Run this file to perform minimal synthesis using NAND or NOR gates.
Will need network.py and node.py files for this to work.
In the main function you can choose to enter a function one at a time or to read a file of function indices.
Look for the ##***  ***## comments to see where the user can make changes.
Set the logic gates to NAND or NOR in the node.py file.  
"""


def FtnToStr(f, inputList):
    """ Helper function
    returns a string of 1s 0s representing the truth table for the function f
    starts at 00..0 row of truth table through 11..1 of truth table
    """

    str = ""
    for minterm in list(iter_points(inputList)):
        val = f.restrict(minterm)
        if val.equivalent(ONE):
            str += "1"
        else:
            str += "0"
    return str


def makeMinterm(inputBdds, integer):
    """ Helper function
        inputBdds is the list of bdds that represent the inputs to the function
        integer is the decimal representation for the current line of the truth table
        Produces a BDD that is true for this line of the truth table
    """
    # start with mintuerm = 1
    minterm = ONE
    # walk over each input variable
    for i in range(len(inputBdds)):
        # if the truth table row is 1 for this variable add the positive variable
        if integer % 2:
            minterm = minterm & inputBdds[i]
        # if the truth table row is 0 for this variable add the negative variable
        else:
            minterm = minterm & ~inputBdds[i]
        integer = integer // 2
    return minterm


def intTobdd(I, inputBdds):
    """ Helper funciton that converts an integer I to a bdd representing the same function.
        Returns the resulting bdd.
        inputBdds is the list of bdds that are inputs for the function
        I is the decimal representation of the binary number determine by the truth table of the function
    """
    # start with the function = 0
    F = ZERO
    # pwr = number of lines in the truth table
    pwr = 2**len(inputBdds)
    # walk over each line of the truth table
    for i in range(0, pwr):
        # if the value of the function is 1 for this line
        if I % 2:
            # add the minterm corresponding to this line of the truth table to F
            F = F | makeMinterm(inputBdds, pwr-i-1)
        I = I // 2
    return F

'''
The ExploreNetwork procedure is the main procedure of the algorithm. It takes as input a single partial network. It 
performs a covering of some uncovered minterm in the partial network and then recursively calls the same procedure using 
the new partial network. 

The pseudocode for the ExploreNetwork procedure:

ExploreNetwork (Network) { 
   if (AllCovered (Network)) {
      Store Network as optimal solution;
      UpperBound ← Network.cost; 
   } else {
      m, Node ← SelectMintermForCovering (Network); 
      Connectible ← FindConnectibleSet(Node, m); 
      for (every C ∈ Connectible) {
         NewNetwork ← PerformCovering (Network, Node, C, m); 
         PropagateFunctionalImplications(NewNode); 
         PropagateFunctionalImplications(NewC); 
         UpdateConnectibleSet(NewNetwork);
         if (NewNetwork.cost < UpperBound) ExploreNetwork (NewNetwork);
      } 
   }
}
'''
def ExploreNetwork (network, UpperBound):
    """
    Recursively generates a solution for the partial network
    Network cost must remain less than UpperBound
    returns the minimum network created from the starting partial network
    return None if no network smaller than UpperBound exists
    """
    ## For Testing Purposes
    #     print("UpperBound = ", UpperBound)
    #     network.PrintNetwork()
    ##

    # If every minterm of each node is covered then network is a solution
    if(network.AllCovered()):
        # Is this the best cost so far?
        if network.cost < UpperBound:
            # Return the network as a minimum solution
            network.PrintNetwork()
            print("Return minimal network with cost", network.cost,"\n")
            return network.cost, network
        else:
            del network
            # Return None because this network is not minimum
            print("Return nothing - network is not minimal")
            return
    # If the network is not a solution then we need to keep building the network
    else:
        bestNetwork = None
        # Find the minterm and node for covering
        node, m = network.SelectMintermForCovering()
        # Find the connectible nodes that can do the covering
        connectible = network.FindConnectibleSet(node, m)
        # if(len(connectible) > 1):
        #     print(network.idx,": Branches =", len(connectible))
        for c in connectible:
            newNetwork = network.PerformCovering(c, node, m)
            # Functional & Structural Implications are done in the PerformCovering Function

            if newNetwork != None and newNetwork.cost < UpperBound:
                # newNetwork.PrintNetwork()
                finishedCost, finishedNetwork = ExploreNetwork(newNetwork, UpperBound)
                if finishedNetwork != None and finishedCost < UpperBound:
                    UpperBound = finishedCost
                    bestNetwork = finishedNetwork
            ## For Testing Purposes
            # else:
            #     print("skip network")
            #     if(newNetwork == None):
            #         print("\terror in network")
            #     else:
            #         newNetwork.PrintNetwork()
            ##
            # Pruning based on overlapped covering
            node.Pruning(c, m)

        if network != bestNetwork:
            del network
        return UpperBound, bestNetwork

def mymain( ):
    ''' Runs the main program '''


    ###### Run a single function at a time ######
    
    # Get number of inputs
    # INPUT = int(input("Enter # of inputs: "))
    INPUT = 4  ##*** ENTER NUMBER OF INPUTS HERE  ***##
    # Create a BDD variable for each input - store them in inputBDDs list
    inputBDDs = []
    for i in range(INPUT):
        var = chr(ord('a')+i)
        print("variable", var, "created")
        var_bdd = exprvar(var)
        inputBDDs.append(var_bdd)
    print("created", len(inputBDDs), "variables")
    
    # Get Function from the user in the form of an integer
    # convert the function index to the corresponding BDD representing the same function
    # store the function in the outputBDDs list
    # only getting 1 function for now - change this so that multiple output functions are possible.
    # outputBDDs = []
    # ftnIndex = int(input("Enter function index:" ))
    ftnIndex = 123  ##*** ENTER FUNCTION HERE  ***##
    f = intTobdd(ftnIndex, inputBDDs)
    outputBDD = f

    ## For Testing Purposes
    print("output function: ")
    # print(ftnIndex)
    print(expr2truthtable(f))
    ##

     # NAND only networks or NOR only networks

    # Make new empty Network
    Original = Network()

    # Initialize network with inputs & outputs
    Original.Initialization(inputBDDs, outputBDD)

    ## For Testing Purposes
    Original.PrintNetwork()
    ##

    # Set UpperBound
    UpperBound = 10
    starttime = time.time()
    BestCost, OptimalSolution = ExploreNetwork(Original, UpperBound)
    endtime = time.time()
    print("Solution found: cost =", BestCost)
    OptimalSolution.PrintNetwork()
    print("Time: ", endtime-starttime)

    
    ###### Run a file function at a time ######
    ##*** CHANGE FILE INPUTS HERE  ***##
    '''
    inFile = open("3inputminterms.txt", "r")
    outFile = open("3inputResults.txt", "w")
    outFile.write("inputs;binary;integer;number of gates;optimal network\n")

    # First Line of the file is the number of inputs
    firstline = inFile.readline()
    INPUT = int(firstline)
    # Create a BDD variable for each input - store them in inputBDDs list
    inputBDDs = []
    for i in range(INPUT):
        var = chr(ord('a')+i)
        print("variable", var, "created")
        var_bdd = exprvar(var)
        inputBDDs.append(var_bdd)
    print("created", len(inputBDDs), "variables")


    # Remaining lines of the file are integer values for the functions
    for line in inFile:
        if line[0] != "#":
            # Get Function from the next line in the file in the form of an integer
            ftnIndex = int(line)

            # convert the function index to the corresponding BDD representing the same function
            f = intTobdd(ftnIndex, inputBDDs)
            # store the function in the outputBDDs list
            outputBDD = f

            ## For Testing Purposes
            print("output function: ",ftnIndex)
            print(expr2truthtable(f))
            ##

            # NAND only networks or NOR only networks

            # Make new empty Network
            Original = Network()

            # Initialize network with inputs & outputs
            Original.Initialization(inputBDDs, outputBDD)

            ## For Testing Purposes
            # Original.PrintNetwork()
            ##

            # Set UpperBound
            UpperBound = 11
            # Start Timer
            starttime = time.time()
            # Run Algorithm to find solution
            BestCost, OptimalSolution = ExploreNetwork(Original, UpperBound)
            # Stop Timer
            endtime = time.time()
            # Print Solution
            print("Solution found: cost =", BestCost)
            OptimalSolution.PrintNetwork()
            print("Time: ", endtime - starttime)
            # Write Solution to file
            outFile.write(str(INPUT)+";"+FtnToStr(f, inputBDDs)+";"+str(ftnIndex)+";"+str(BestCost)+";")
            OptimalSolution.WriteNetwork(outFile)



    inFile.close()
    outFile.close()
    '''
mymain()

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <stack>

#include "util.h"
#include "cudd.h"
#include "cuddObj.hh"

using namespace std;

//Data Structures
struct Node;
class Network;

ofstream status;

int StopTime = 60 * 60 * 48; //hours

bool skipToEnd = false;

bool PrintCatalog = false;

//Heuristics
char MintermChoice = 'H';  //Best option = 'M','J','H' - Options 'A' - 'R', 'S'
bool DuplCover = true;     //true if using duplicate covering removal
bool GateSymm = true;      //true if using gate symmetry (update unconnectible)
bool symmetry = true;      //true if using symmetries on input variables
bool StrucImpl = false;     //true if add new gates after covering is complete
char NetworkOrder = 'C';   //Best option = 'C'
                                          //A = Explore Networks as they're created, connectible order: primary inputs, fan-in, gate nodes, new
                                          //B = Explore Network as they're created, connectible order: fan-in, primary inputs, gate nodes, new
                                          //C = Order Networks by cost (small to large), connectible order: primary inputs, fan-in, gate nodes, new
                                          //D = Order Networks by cost (small to large), connectible order: fan-in, primary inputs, gate nodes, new
                                          //E = Order Networks by cost (small to large), connectible order: Random
                                          //F = Explore Network as they're created, connectible order: Random

bool TreeFirst = false; //Can't guarantee inital solution with fan-in restriction unless TRUE
bool ExtendedBridgeC = false;
bool ExtendedBridgeAll = false;
bool SimpleBridge = false;

//Relaxation of UpperBound
char RelaxVer = 'A';         //A: No Relaxation Done
int RelaxConst = 500;        //B: Lower UB by constant (RelaxConst) each time
float RelaxPercent = 2;      //C: Paramertize relaxation by number of branches: After n = RelaxConst branches, reduce Upper Bound by constant RelaxPercent
int RelaxDone = 0;           //D: Lower UB by % (RelaxPercent) each time
ofstream Subopt;             //E: Paramertize relaxation by number of branches: After n = RelaxConst branches, reduce Upper Bound by % RelaxPercent
                             //F: Stop search after RelaxConst seconds

//Output
bool print = false;
bool trace = false;
bool all = false;
bool onesol = true;

//Counter for number of networks created
int ComputeNetworkCount;
int BranchCount;
int ImplicationCount;
int LeafCount;
int StrucImplCount;

//Bdds
int INPUT;
BDD ZERO;
BDD ONE;
Cudd* Manager;
vector<BDD> InputBdds;

//Globals
int INF = 10000;
int FaninBound = 2;
int GateType = 5;  //1: 'nand' 2: 'nor' 3: 'and/or/not' 4: 'and/or' - must use complemented inputs; 5: 'nand/nor'
vector<Network*> Solutions;

//Variations
bool LevelRestrict = false;
int  LevelRestriction = 3;
int  gateRank = 1;
int  edgeRank = 0;
int  levelRank = 0;
bool FanoutRestrict = false;
int FanoutRestriction = 3;
bool LevelBoundCheck = false;
int LevelBound;
bool ComplementedInputs = true;
bool FirstNetwork = false;

//Counters
int NODECOUNT;
int NETWORKCOUNT;
int CONFLICTCOUNT;
int ExtendedBridgeCount;
int SimpleBridgeCount;

//BDD Functions
bool BddLeq(BDD,BDD);  /* !! */
BDD MakeMinterm(vector<BDD>&, int); /* !!! */
int Number(BDD); /* !!! */
void PrintInterval(BDD, BDD, ostream& out = cout); /* !!! */
void PrintFunction(BDD, ostream& out = cout); /* !!! */
void PrintUncoveredFunction(Node*, BDD, ostream& out = cout);
void PrintTerm(BDD, ostream& out = cout); /* !!! */
BDD PickMinterm(BDD f); /* !!! */
//BDD PickMinterm(BDD, Node*, vector<Node*>& ); /* !!! */
bool MoreOnes(BDD);

//Conversion Functions
BDD intTobdd(int); /* !!! */
string intTostring(int); /* !!! */
string floatTostring(float);
long stringToint (string); /* !!! */
BDD stringTobdd(string); /* !!! */
void ReadBlifFile(ifstream&, vector<BDD>&); /* !!! */
void ReadRandomFile(ifstream&, vector<BDD>&);

//Node Functions
bool StructureCompatible(Node* N, Node* C); /* !!! */
bool OffSetCompatible(Node* N, Node* C);  /* !!! */
bool OnSetCompatible(Node* N, Node* C);  /* !!! */
bool OffSetCompatible(Node* N, BDD);  /* !!! */
bool OnSetCompatible(Node* N, BDD);  /* !!! */
bool Ancestor(Node*, Node*); /* !!! */
Node* CopyNode(Node*,int); /* !!! */
Node* newNode(char LogicOp = '-'); /* !!! */
void PrintNode(Node*, ostream& out = cout); /* !!! */
void PrintSimpleNode(Node*, ostream&); /* !!! */
bool UpdateOnSet(Node* , BDD , Node*, int); /* !!! */ 
bool UpdateOffSet(Node* , BDD , Node*, int); /* !!! */ 
bool UpdateOnMap(Node* , BDD , map<Node*, BDD>& , map<Node*, BDD>& );
bool UpdateOffMap(Node* , BDD , map<Node*, BDD>& , map<Node*, BDD>& );
void SetLevel(Node*, int); /* !!! */
int CountConn(Node*);
int FindInput(Node*, Node*);
void UpdateLabel(Node*);
int GetLabel(Node*, BDD&);
int GetGateLabel(Node*);
//////////////////////////////////////////////////////////////////////////////
// DataStructures
//////////////////////////////////////////////////////////////////////////////
struct Node {
    int idx;
    int level;
    BDD ON;
    BDD OFF;
    BDD Uncovered;
    vector<Node*> Inputs;
    set<Node*> Parents;
    set<Node*> AlreadyConnected;
    vector<Node*> Connectible;
    int Label;
    vector<BDD> UncoveredSet;
    char LogicOp;  //Only used if GateType == '3' or '4' or '5'.  'A' -> AND Gate, 'O' -> OR Gate, 'N' -> NOT Gate, 'D' -> NAND, 'R' -> NOR
    
    friend ostream& operator<<(ostream& out, const Node* N) {
	if(N == NULL) out<<"NULL";
	else if(N->idx < 0) {
	    out<<"new";
	    if((GateType == 3 || GateType == 4) && N->LogicOp == 'A') out<<"(A)";
	    else if((GateType == 3  || GateType == 4) && N->LogicOp == 'O') out<<"(O)";
	    else if(GateType == 3 && N->LogicOp == 'N') out<<"(N)";
	    else if(GateType == 5 && N->LogicOp == 'D') out<<"(D)";
	    else if(GateType == 5 && N->LogicOp == 'R') out<<"(R)";
	    else out<<"";
	} else {
	    if(N->IsInput()) { 
		if(!ComplementedInputs) out<<"x_"<<N->idx; 
		else 
		    if(N->idx % 2 == 0) out<<"x_"<<N->idx;
		    else out<<"-x_"<<N->idx-1;
	    } else out<<"g_"<<N->idx;
	}
    }
    Node() {
	ON = ZERO; OFF = ZERO; ON = ZERO; Label = 0; 
	for(int i = 0; i < 5; i++)
	    UncoveredSet.push_back(ZERO);
    }
    string Name() { 
	if(idx < 0) {
	    return "new";
	} else {
	    if(IsInput()) 	
		if(!ComplementedInputs) return "x_" + intTostring(idx); 
		else 
		    if(idx % 2 == 0) return "x_" + intTostring(idx);
		    else return "-x_" + intTostring(idx-1);
 	    else return "g_" + intTostring(idx);
	}
    }
    bool IsInput() const {
	if(idx < 0) return false;
	else if(idx < INPUT || (ComplementedInputs && idx < 2*INPUT)) return true;
	else return false;
    }
   bool IsFanin(Node* N) {
       for(int i = 0; i < Inputs.size(); i++)
	   if(Inputs[i] == N) return true;
       return false; 
   }
};

class Network {
  long idx;
  vector<Node*> Outputs;
  vector<Node*> Inputs;
  vector<Node*> Internal;
  int cost;
  map<Node*,set<Node*> > SymmetryPairs;
  int vars;

 private:
  void PrintPicRec(int, Node*, vector<string>&, set<int>&); /* !!! */

 public:
  Network(){idx = NETWORKCOUNT++; vars = 0;};
  ~Network();
  Network* CopyNetwork(map<Node*,Node*>&); /* !!! */
  Network* AddNodeInput(Node*, Node*, BDD, map<Node*,Node*>&); /* !!! */

  //MANIPULATORS 
  void AddNewNodes();
  Node* AddNewNodeInput(Node*, BDD);
  void ComputeSymmetries(); /* !!! */
  void InsertInput(BDD); /* !!! */
  void InsertOutput(BDD,char LogicOp = '-'); /* !!! */
  void Initialization(); /* !!! */
  void SetCount(int ct) {cost = ct;}; //Sets the cost of the network to ct /* !!! */
  void UpdateConnectibleSets();  
  void UpdateUncovered();
  void CompleteTree(Node*, map<Node*,BDD>&, map<Node*, BDD>&);
  bool ExtendedBridgeImpl(Node*, Node*, bool);
  bool SimpleBridgeImpl(Node*, Node*);
  int  ComputeCost();

  //OUTPUT
  void PrintPicture(ostream& out = cout); /* !!! */
  void PrintText(ostream& out = cout);
  void PrintSimpleSolution(bool, ostream& out = cout); /* !!! */
  void PrintOutput(ostream& out = cout); /* !!! */
  void PrintCatalog(ostream& out = cout);
  string PrintFanin() {
      int minFanin = INF;
      int maxFanin = 0;
      float avgFanin = 0;
      int total = 0;
      for(int i = 0; i < Outputs.size(); i++) {
	  int Fanin = Outputs[i]->Inputs.size();
	  if(Fanin < minFanin) minFanin = Fanin;
	  if(Fanin > maxFanin) maxFanin = Fanin;
	  avgFanin += Fanin;
	  total++;
      }
      for(int i = 0; i < Internal.size(); i++) {
	  int Fanin = Internal[i]->Inputs.size();
	  if(Fanin < minFanin) minFanin = Fanin;
	  if(Fanin > maxFanin) maxFanin = Fanin;
	  avgFanin += Fanin;
	  total++;
      }
      avgFanin = avgFanin / total;
      string response = intTostring(minFanin) + ";" + floatTostring(avgFanin) + ";" + intTostring(maxFanin);
      return response;
  }

  //ACCESSORS
  vector<Node*> FindConnectible(Node* N); /* !!! */
  int Cost() {return cost;}; /* !!! */
  int LowerBound();
  vector<Node*> RemoveSymmetric(Node*, vector<Node*>&, map<Node*,vector<Node*> >& );  /* !!! */
  int Level(); /* !!! */
  int Gates();
  int Edges();
  int GetNumber() { return Number(Outputs[0]->ON); }
  bool IsInput(Node* N) {for(int i = 0; i < Inputs.size(); i++) if(N == Inputs[i]) return true; return false;}
  bool IsSymmetric(BDD, BDD); /* !!! */
  int GetIdx() {return idx;}
  Node* FindNewNodeNeeded(BDD&);
  int InputCount() {return Inputs.size();}
  int OutputCount() {return Outputs.size();}
  vector<BDD> GetInputBdds() {
      vector<BDD> inputbdds;
      for(int i = 0; i < Inputs.size(); i++)
	  inputbdds.push_back(Inputs[i]->ON);
      return inputbdds;
  }
  bool FindUncoveredMaxInput(Node*&, set<Node*>&);
  bool TreeStructure();
  bool UncoveredNode();
  BDD SelectMintermForCovering(Node*&, vector<Node*>&);
  set<Node*> Symmetric(Node* C) { return SymmetryPairs[C]; }
  bool Duplicates();
  bool SymmetricSol(vector<Network*>&);

  int CountUncovered() {
      int ct = 0;
      for(int i = 0; i < Outputs.size(); i++)
	  if(Outputs[i]->Uncovered != ZERO) ct++;
      for(int i = 0; i < Internal.size(); i++)
	  if(Internal[i]->Uncovered != ZERO) ct++;
      return ct;
  }
  bool OrFirst() {
      if(Outputs[0]->LogicOp == 'O') return true;
      else return false;
  }
  bool AndFirst() {
      if(Outputs[0]->LogicOp == 'A') return true;
      else return false;
  }
};

void GetCSF(Node* N,map<Node*, BDD>& CSF) {
    BDD F = ZERO;
    for(int i = 0; i < N->Inputs.size(); i++) {
	if(CSF.find(N->Inputs[i]) == CSF.end()) GetCSF(N->Inputs[i], CSF);
	F += !CSF[N->Inputs[i]];
    }
    CSF[N] = F;
}

bool Network::Duplicates() {
    map<Node*, BDD> CSF;
    for(int j = 0; j < Inputs.size(); j++)
	CSF[Inputs[j]] = Inputs[j]->ON;
    for(int i = 0; i < Outputs.size(); i++) {
	GetCSF(Outputs[i], CSF);
    }
    for(int i = 0; i < Internal.size(); i++) {
	Node* N = Internal[i];
	for(int j = 0; j < Inputs.size(); j++)
	    if(CSF[N] == CSF[Inputs[j]]) return true;
	for(int j = i+1; j < Internal.size(); j++)
	    if(CSF[N] == CSF[Internal[j]]) return true;
	for(int j = 0; j < Outputs.size(); j++)
	    if(CSF[N] == CSF[Outputs[j]]) return true;
    }
    return false;
}

//Only Fan-in = 2 works
bool AreSymmetric(Node* N1, Node* N2, vector<map<Node*,Node*> >& directories) {
    //cout<<"AreSymmetric("<<N1<<", "<<N2<<", "<<directories.size()<<")"<<endl;

    if(N1->Inputs.size() != N2->Inputs.size()) {
	//cout<<"  "<<N1->Inputs.size()<<" != "<<N2->Inputs.size()<<" -> False"<<endl;
	return false;
    }

    vector<map<Node*, Node*> > NewDirectories;
    for(int i = 0; i < directories.size(); i++) {
	if(directories[i].find(N1) != directories[i].end()) {
	    if(directories[i][N1] == N2) {
		NewDirectories.push_back(directories[i]);
		//cout<<"  keep directory "<<i<<": "<<N1<<" = "<<N2<<endl;
		//} else {
		//cout<<"  remove directory "<<i<<": "<<N1<<" != "<<N2<<endl;
	    }
	} else {
	    directories[i][N1] = N2;
	    NewDirectories.push_back(directories[i]);
	    //cout<<"  keep directory "<<i<<": "<<N1<<" -> "<<N2<<endl;
	}
    }
    directories = NewDirectories;
    if(directories.empty()) {
	//cout<<"  no directories left -> False"<<endl;
	return false;
    }

    if(!N1->Inputs.empty()) {
	NewDirectories.clear();
	int input_comb = N1->Inputs.size();
	for(int i = 0; i < input_comb; i++) {
	    if(i == 0) {
		vector<map<Node*, Node*> > Dir_i = directories;
		bool keep = true;
		for(int j = 0; j < N1->Inputs.size() && keep; j++) {
		    //cout<<"Try "<<N1<<", "<<N2<<" -> "<<N1->Inputs[j]<<", "<<N2->Inputs[j]<<endl;
		    if(!AreSymmetric(N1->Inputs[j], N2->Inputs[j], Dir_i)) keep = false;
		    //cout<<"End "<<N1<<", "<<N2<<" -> "<<N1->Inputs[j]<<", "<<N2->Inputs[j]<<endl;
		}
		if(keep) 
		    for(int j = 0; j < Dir_i.size(); j++)
			NewDirectories.push_back(Dir_i[j]);
	    } else {
		vector<map<Node*, Node*> > Dir_i = directories;
		bool keep = true;
		for(int j = 0; j < N1->Inputs.size() && keep; j++) {
		    //cout<<"Try "<<N1<<", "<<N2<<" -> "<<N1->Inputs[j]<<", "<<N2->Inputs[input_comb-j-1]<<endl;
		    if(!AreSymmetric(N1->Inputs[j], N2->Inputs[input_comb-j-1], Dir_i)) keep = false;
		    //cout<<"End "<<N1<<", "<<N2<<" -> "<<N1->Inputs[j]<<", "<<N2->Inputs[input_comb-j-1]<<endl;
		}
		if(keep)
		    for(int j = 0; j < Dir_i.size(); j++)
			NewDirectories.push_back(Dir_i[j]);
	    }
	}
	directories = NewDirectories;
    }
    //cout<<"Finished Symmetric "<<N1<<", "<<N2<<", "<<NewDirectories.size()<<endl;
    if(directories.empty()) return false;
    else return true;
}

bool Network::SymmetricSol(vector<Network*>& Solutions) {

    for(int i = 0; i < Solutions.size(); i++) {
	vector<map<Node*, Node*> > directory;
	map<Node*,Node*> initial; directory.push_back(initial);
	bool FoundSymm = true;
	//PrintPicture();
	//Solutions[i]->PrintPicture();
	//cout<<"Ready of check > "; cin.get();
	for(int j = 0; j < Outputs.size(); j++)
	    FoundSymm *= AreSymmetric(Outputs[j], Solutions[i]->Outputs[j], directory);
	//cout<<"Finished - "<<FoundSymm<<" > "; cin.get();
	if(FoundSymm) return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////////
// Network Functions
//////////////////////////////////////////////////////////////////////////////
Network::~Network() {
  for(int i = 0; i < Outputs.size(); i++) 
    delete Outputs[i];
  for(int i = 0; i < Inputs.size(); i++) 
    delete Inputs[i];
  for(int i = 0; i < Internal.size(); i++) 
    delete Internal[i];
}

/* !!! */
//Copies the Network by creating all new nodes.
//The map directories maps the old nodes to the new nodes
Network* Network::CopyNetwork(map<Node*,Node*>& directory) {
  Network* newNet = new Network;
  //Copy Outputs
  for(int i = 0; i < Outputs.size(); i++) {
    Node* N = CopyNode(Outputs[i], Outputs[i]->idx);
    newNet->Outputs.push_back(N);
    directory[Outputs[i]] = N;
  }
  //Copy Inputs
  for(int i = 0; i < Inputs.size(); i++) {
    Node* N = CopyNode(Inputs[i], Inputs[i]->idx);
    newNet->Inputs.push_back(N);
    directory[Inputs[i]] = N;
  }
  //Copy Internal
  for(int i = 0; i < Internal.size(); i++) {
    Node* N = CopyNode(Internal[i], Internal[i]->idx);
    newNet->Internal.push_back(N);
    directory[Internal[i]] = N;
  }
  //Copy Symmetries
  for(map<Node*,set<Node*> >::iterator i = SymmetryPairs.begin(); i != SymmetryPairs.end(); i++) {
      for(set<Node*>::iterator j = (*i).second.begin(); j != (*i).second.end(); j++) {
	  newNet->SymmetryPairs[directory[(*i).first]].insert(directory[*j]);
      }
  }
  set<Node*> temp;
  //Update Inputs, Parents, Already Connected of Output Nodes
  for(int i = 0; i < Outputs.size(); i++) {
      for(int j = 0; j < Outputs[i]->Inputs.size(); j++)
	  directory[Outputs[i]]->Inputs.push_back(directory[Outputs[i]->Inputs[j]]);
      for(set<Node*>::iterator j = Outputs[i]->Parents.begin(); j != Outputs[i]->Parents.end(); j++)
	  temp.insert(directory[*j]);
      directory[Outputs[i]]->Parents = temp;
      for(int j = 0; j < Outputs[i]->Connectible.size(); j++)
	  directory[Outputs[i]]->Connectible.push_back(directory[Outputs[i]->Connectible[j]]);
      temp.clear();
      for(set<Node*>::iterator j = Outputs[i]->AlreadyConnected.begin(); j != Outputs[i]->AlreadyConnected.end(); j++)
	  temp.insert(directory[*j]);
      directory[Outputs[i]]->AlreadyConnected = temp;
      temp.clear();
  }
  //Update Inputs, Parents of Internal Nodes
  for(int i = 0; i < Internal.size(); i++) {
      for(int j = 0; j < Internal[i]->Inputs.size(); j++)
	  directory[Internal[i]]->Inputs.push_back(directory[Internal[i]->Inputs[j]]);
      for(set<Node*>::iterator j = Internal[i]->Parents.begin(); j != Internal[i]->Parents.end(); j++)
	  temp.insert(directory[*j]);
      directory[Internal[i]]->Parents = temp;
      for(int j = 0; j < Internal[i]->Connectible.size(); j++)
	  directory[Internal[i]]->Connectible.push_back(directory[Internal[i]->Connectible[j]]);
      temp.clear();
      for(set<Node*>::iterator j = Internal[i]->AlreadyConnected.begin(); j != Internal[i]->AlreadyConnected.end(); j++)
	  temp.insert(directory[*j]);
      directory[Internal[i]]->AlreadyConnected = temp;
      temp.clear();
  }
  //Update Parents of Input Nodes
  for(int i = 0; i < Inputs.size(); i++) {
    for(set<Node*>::iterator j = Inputs[i]->Parents.begin(); j != Inputs[i]->Parents.end(); j++)
      temp.insert(directory[*j]);
    directory[Inputs[i]]->Parents = temp;
    temp.clear();
 }
  //Update Cost
  newNet->cost = cost;
  //Update Vars
  newNet->vars = vars;
  return newNet;
}

/* !!! */
void SetLevel(Node* C, int value) {
  C->level = value;
  for(int i = 0; i < C->Inputs.size(); i++) 
      if(value+1 > C->Inputs[i]->level) SetLevel(C->Inputs[i], value+1);
}

set<Node*> GetChildren(Node* N) {
    set<Node*> Children;
    Children.insert(N);
    for(int i = 0; i < N->Inputs.size(); i++) {
	set<Node*> InputChildren = GetChildren(N->Inputs[i]);
	Children.insert(InputChildren.begin(), InputChildren.end());
    }
    return Children;
}

set<Node*> GetAncestors(Node* N) {
    set<Node*> Ancestors;
    Ancestors.insert(N);
    for(set<Node*>::iterator i = N->Parents.begin(); i != N->Parents.end(); i++) {
	set<Node*> TempAncestors = GetAncestors(*i);
	Ancestors.insert(TempAncestors.begin(), TempAncestors.end());
    }
    return Ancestors;
}

set<Node*> FindAllDescendents(Node* N) {
    set<Node*> Descendents;
    Descendents.insert(N);
    for(int i = 0; i < N->Inputs.size(); i++) {
	set<Node*> TempDescendents = FindAllDescendents(N->Inputs[i]);
	Descendents.insert(TempDescendents.begin(), TempDescendents.end());
    }
    return Descendents;
}

void Network::CompleteTree(Node* N, map<Node*,BDD>& OnMap, map<Node*, BDD>& OffMap) {
    if(OnMap.find(N) == OnMap.end()) { OnMap[N] = ZERO;	OffMap[N] = ZERO; }
    else return;
    if(IsInput(N)) { OnMap[N] = N->ON; OffMap[N] = N->OFF; }
    else if(N->Inputs.size() > 0) {
	BDD ON, OFF;
	if(GateType == 1) {ON = ZERO; OFF = ONE;}
	else if(GateType == 2) {ON = ONE; OFF = ZERO;}
	else if(GateType == 3) {
	    if(N->LogicOp == 'A')      {ON = ONE;  OFF = ZERO;}
	    else if(N->LogicOp == 'O')  {ON = ZERO; OFF = ONE; }
	    else if(N->LogicOp == 'N') {ON = ZERO; OFF = ZERO;}
	} else if(GateType == 4) {
	    if(N->LogicOp == 'A')      {ON = ONE;  OFF = ZERO;}
	    else if(N->LogicOp == 'O')  {ON = ZERO; OFF = ONE; }
	} else if(GateType == 5) {
	  if(N->LogicOp == 'D')  {ON = ZERO; OFF = ONE;}
	  else if(N->LogicOp == 'R') {ON = ONE; OFF = ZERO;}
	}
	for(int i = 0; i < N->Inputs.size(); i++) {
	    CompleteTree(N->Inputs[i], OnMap, OffMap);
	    if(GateType == 1) {
		ON += OffMap[N->Inputs[i]];
		OFF *= OnMap[N->Inputs[i]];
	    } else if(GateType == 2) {
		ON *= OffMap[N->Inputs[i]];
		OFF += OnMap[N->Inputs[i]];
	    } else if(GateType == 3) {
		if(N->LogicOp == 'A') {
		    ON *= OnMap[N->Inputs[i]];
		    OFF += OffMap[N->Inputs[i]];
		} else if(N->LogicOp == 'O') {
		    ON += OnMap[N->Inputs[i]];
		    OFF *= OffMap[N->Inputs[i]];
		} else if(N->LogicOp == 'N') {
		    ON = OffMap[N->Inputs[i]];
		    OFF = OnMap[N->Inputs[i]];
		}
	    } else if(GateType == 4) {
		if(N->LogicOp == 'A') {
		    ON *= OnMap[N->Inputs[i]];
		    OFF += OffMap[N->Inputs[i]];
		} else if(N->LogicOp == 'O') {
		    ON += OnMap[N->Inputs[i]];
		    OFF *= OffMap[N->Inputs[i]];
		}
	    } else if(GateType == 5) {
	      if(N->LogicOp == 'D') {
		ON += OffMap[N->Inputs[i]];
		OFF *= OnMap[N->Inputs[i]];
	      } else if(N->LogicOp == 'R') {
		ON *= OffMap[N->Inputs[i]];
		OFF += OnMap[N->Inputs[i]];
	      }
	    }
	}
	OnMap[N] = ON; OffMap[N] = OFF;
    }
}

void Network::UpdateUncovered() {
    map<Node*,BDD> OnMap;
    map<Node*,BDD> OffMap;
    for(int i = 0; i < Outputs.size(); i++) 
	CompleteTree(Outputs[i], OnMap, OffMap);
    for(int i = 0; i < Outputs.size(); i++) {
	Node* N = Outputs[i];
	BDD PrevUncovered = N->Uncovered;
	if(GateType == 1) N->Uncovered = N->ON;
	else if(GateType == 2) N->Uncovered = N->OFF;
	else if(GateType == 3) {
	    if(N->LogicOp == 'A') N->Uncovered = N->OFF;
	    else if(N->LogicOp == 'O') N->Uncovered = N->ON;
	    else if(N->LogicOp == 'N') N->Uncovered = N->ON;
	} else if(GateType == 4) {
	    if(N->LogicOp == 'A') N->Uncovered = N->OFF;
	    else if(N->LogicOp == 'O') N->Uncovered = N->ON;
	} else if(GateType == 5) {
	  if(N->LogicOp == 'D') N->Uncovered = N->ON;
	  else if(N->LogicOp == 'R') N->Uncovered = N->OFF;
	}
	for(int i = 0; i < N->Inputs.size(); i++) {
	    if(GateType == 1) N->Uncovered *= !N->Inputs[i]->OFF;
	    else if(GateType == 2) N->Uncovered *= !N->Inputs[i]->ON;
	    else if(GateType == 3) {
		if(N->LogicOp == 'A') N->Uncovered *= !N->Inputs[i]->OFF;
		else if(N->LogicOp == 'O') N->Uncovered *= !N->Inputs[i]->ON;
		else if(N->LogicOp == 'N') N->Uncovered *= !N->Inputs[i]->OFF;
	    } else if(GateType == 4) {
		if(N->LogicOp == 'A') N->Uncovered *= !N->Inputs[i]->OFF;
		else if(N->LogicOp == 'O') N->Uncovered *= !N->Inputs[i]->ON;
	    } else if(GateType == 5) {
	      if(N->LogicOp == 'D') N->Uncovered *= !N->Inputs[i]->OFF;
	      else if(N->LogicOp == 'R') N->Uncovered *= !N->Inputs[i]->ON;
	    }
	}
	if(GateType == 1) N->Uncovered *= !OnMap[N];
	else if(GateType == 2) N->Uncovered *= !OffMap[N];
	else if(GateType == 3) {
	    if(N->LogicOp == 'A') N->Uncovered *= !OffMap[N];
	    else if(N->LogicOp == 'O') N->Uncovered *= !OnMap[N];
	    else if(N->LogicOp == 'N') N->Uncovered *= !OnMap[N];
	} else if(GateType == 4) {
	    if(N->LogicOp == 'A') N->Uncovered *= !OffMap[N];
	    else if(N->LogicOp == 'O') N->Uncovered *= !OnMap[N];
	} else if(GateType == 5) {
	  if(N->LogicOp == 'D') N->Uncovered *= !OnMap[N];
	  else if(N->LogicOp == 'R') N->Uncovered *= !OffMap[N];
	}
	if(print && N->Uncovered != PrevUncovered) {
	    cout<<"  Update Uncovered: "<<N<<" = "; PrintUncoveredFunction(N, N->Uncovered); cout<<endl;
	}
    }
    for(int i = 0; i < Internal.size(); i++) {
	Node* N = Internal[i];
	BDD PrevUncovered = N->Uncovered;
	if(GateType == 1) N->Uncovered = N->ON;
	else if(GateType == 2) N->Uncovered = N->OFF;
	else if(GateType == 3) {
	    if(N->LogicOp == 'A') N->Uncovered = N->OFF;
	    else if(N->LogicOp == 'O') N->Uncovered = N->ON;
	    else if(N->LogicOp == 'N') N->Uncovered = N->ON;
	} else if(GateType == 4) {
	    if(N->LogicOp == 'A') N->Uncovered = N->OFF;
	    else if(N->LogicOp == 'O') N->Uncovered = N->ON;
	} else if(GateType == 5) {
	  if(N->LogicOp == 'D') N->Uncovered = N->ON;
	  if(N->LogicOp == 'R') N->Uncovered = N->OFF;
	}
	for(int i = 0; i < N->Inputs.size(); i++) {
	    if(GateType == 1) N->Uncovered *= !N->Inputs[i]->OFF;
	    else if(GateType == 2) N->Uncovered *= !N->Inputs[i]->ON;
	    else if(GateType == 3) {
		if(N->LogicOp == 'A') N->Uncovered *= !N->Inputs[i]->OFF;
		else if(N->LogicOp == 'O') N->Uncovered *= !N->Inputs[i]->ON;
		else if(N->LogicOp == 'N') N->Uncovered *= !N->Inputs[i]->OFF;
	    } else if(GateType == 4) {
		if(N->LogicOp == 'A') N->Uncovered *= !N->Inputs[i]->OFF;
		else if(N->LogicOp == 'O') N->Uncovered *= !N->Inputs[i]->ON;
	    } else if(GateType == 5) {
	      if(N->LogicOp == 'D') N->Uncovered *= !N->Inputs[i]->OFF;
	      if(N->LogicOp == 'R') N->Uncovered *= !N->Inputs[i]->ON;
	    }
	}
	if(GateType == 1) N->Uncovered *= !OnMap[N];
	if(GateType == 2) N->Uncovered *= !OffMap[N];
	else if(GateType == 3) {
	    if(N->LogicOp == 'A') N->Uncovered *= !OffMap[N];
	    else if(N->LogicOp == 'O') N->Uncovered *= !OnMap[N];
	    else if(N->LogicOp == 'N') N->Uncovered *= !OnMap[N];
	} else if(GateType == 4) {
	    if(N->LogicOp == 'A') N->Uncovered *= !OffMap[N];
	    else if(N->LogicOp == 'O') N->Uncovered *= !OnMap[N];
	} else if(GateType == 5) {
	  if(N->LogicOp == 'D') N->Uncovered *= !OnMap[N];
	  else if(N->LogicOp == 'N') N->Uncovered *= !OffMap[N];
	}
	if(print && N->Uncovered != PrevUncovered) {
	    cout<<"  Update Uncovered: "<<N<<" = "; PrintUncoveredFunction(N, N->Uncovered); cout<<endl;
	}
    }
}
   
int FindInput(Node* P, Node* C) {
    for(int i = 0; i < P->Inputs.size(); i++) {
	if(P->Inputs[i] == C) return i;
    }
    return -1;
}

int Network::LowerBound() {
    int LB = cost;
    if(GateType == 4 && FaninBound == INF) {
	bool NewGate = false;
	for(int i = 0; i < Outputs.size(); i++) {
	    int Label = GetGateLabel(Outputs[i]);
	    if(gateRank > 0 && !NewGate && Label >= 8) LB += gateRank;   //At least one new gate is needed
	    if(edgeRank > 0) {
		if(Outputs[i]->Inputs.size() == 0) LB += edgeRank * 2;   //Every gate must have at least 2 inputs
		else if(Outputs[i]->Inputs.size() == 1) LB += edgeRank;  //Every gate must have at least 2 inputs
		else if(Outputs[i]->Inputs.size() > 1 && Label > 1 && Label != 5) LB += edgeRank; //New edge must be added to gate
	    }
	}
	for(int i = 0; i < Internal.size(); i++) {
	    int Label = GetGateLabel(Internal[i]);
	    if(gateRank > 0 && !NewGate && Label >= 8) LB += gateRank;   //At least one new gate is needed
	    if(edgeRank > 0) {
		if(Internal[i]->Inputs.size() == 0) LB += edgeRank * 2;   //Every gate must have at least 2 inputs
		else if(Internal[i]->Inputs.size() == 1) LB += edgeRank;  //Every gate must have at least 2 inputs
		else if(Internal[i]->Inputs.size() > 1 && Label > 1 && Label != 5) LB += edgeRank; //New edge must be added to gate
	    }
	}
	if(edgeRank > 0 && NewGate) LB += edgeRank * 2;  //New gate must have at least 2 inputs
    }
    return LB;
}

int Network::ComputeCost() {
    int gateCount = 0;
    if(gateRank > 0) gateCount = Gates();
    int edgeCount = 0;
    if(edgeRank > 0) edgeCount = Edges();
    int levelCount = 0;
    if(levelRank > 0) levelCount = Level();
    return ((gateRank*gateCount) + (edgeRank*edgeCount) + (levelRank*levelCount));
}
     
//Create a new network with C covering someting of P.  Update the on/off-sets based on this covering
Network* Network::AddNodeInput(Node* P, Node* C, BDD SelectedMinterm, map<Node*,Node*>& directory) {
  //Copy Network
  Network* newNet = CopyNetwork(directory);

  //Get the new nodes for P and C
  Node* newP = directory[P];
  Node* newC;
  if(C->idx == -1) {
    newC = CopyNode(C, newNet->vars++);
    directory[C] = newC;
    newNet->Internal.push_back(newC);
  } else newC = directory[C];
 
  //Add C as an input of P if necessary
  if(FindInput(newP, newC) < 0 && P->Inputs.size() < FaninBound) {
      newP->Inputs.push_back(newC);
      newC->Parents.insert(newP);
  }

  //Update C's level
  if(newP->level+1 > newC->level) SetLevel(newC, newP->level+1);
  bool UpdateOK = false;

  newNet->cost = newNet->ComputeCost();

  if(GateType == 1) {
      //ON(C) += OFF(P)
      if(UpdateOnSet(newC, newP->OFF, NULL, 0)) {
	  //ON(P) += OFF(C)
	  if(UpdateOnSet(newP, newC->OFF, NULL, 0)) {
	      //OFF(C) += SelectedMinterm - Perform Covering
	      if(UpdateOffSet(newC, SelectedMinterm, NULL, 0)) {
		  //If P has two children, additional updates are possible
		  if(newP->Inputs.size() == FaninBound) {
		      //OFF(P) += {*ON(I) : For all I in INPUT(P)}    Two inputs: OFF(P) = ON(C) * ON(S)
		      BDD OnProd = ONE;
		      for(int i = 0; i < newP->Inputs.size(); i++) OnProd *= newP->Inputs[i]->ON;
		      if(UpdateOffSet(newP, OnProd, NULL, 0)) {
			  //OFF(C) += ON(P) * {*ON(I) : For all I in INPUT(P), where I != C}    Two inputs: OFF(C) += ON(P) * ON(S)
			  OnProd = newP->ON;
			  for(int i = 0; i < newP->Inputs.size(); i++) if(newP->Inputs[i] != newC) OnProd *= newP->Inputs[i]->ON;
			  if(UpdateOffSet(newC, OnProd, NULL, 0)) {
			      //OFF(S) += ON(P) * {*ON(I) : For all I in INPUT(P), where I != S}  Two inputs: OFF(S) += ON(P) * ON(C)
			      UpdateOK = true;
			      for(int i = 0; i < newP->Inputs.size() && UpdateOK; i++) {
				  Node* S = newP->Inputs[i];
				  if(S != C) {
				      OnProd = newP->ON;
				      for(int j = 0; j < newP->Inputs.size(); j++)
					  if(newP->Inputs[j] != S) OnProd *= newP->Inputs[j]->ON;
				      if(UpdateOffSet(S, OnProd, NULL, 0))
					  UpdateOK = true;
				      else { 	
					  if(print) cout<<"  Conflict updating off set of "<<S<<endl;
					  if(trace) status<<"  Conflict updating off set of "<<S<<endl;
					  UpdateOK = false;
				      }
				  }
			      }
			  } else {
			      if(print) cout<<"  Conflict updating off set of "<<newC<<endl;
			      if(trace) status<<"  Conflict updating off set of "<<newC<<endl;
			  }
		      } else {
			  if(print) cout<<"  Conflict updating off set of "<<newP<<endl;
			  if(trace) status<<"  Conflict updating off set of "<<newP<<endl;
		      }
		  } else UpdateOK = true;
	      } else {
		  if(print) cout<<"  Conflict updating off set of "<<newC<<endl;
		  if(trace) status<<"  Conflict updating off set of "<<newC<<endl;
	      }
	  } else {
	      if(print) cout<<"  Conflict updating on set of "<<newP<<endl;
	      if(trace) status<<"  Conflict updating on set of "<<newP<<endl;
	  }
      } else {
	  if(print) cout<<"  Conflict updating on set of "<<newC<<endl;
	  if(trace) status<<"  Conflict updating on set of "<<newC<<endl;
      }
  } else if(GateType == 2) {
      //OFF(C) += ON(P)
      if(UpdateOffSet(newC, newP->ON, NULL, 0)) {
	  //OFF(P) += ON(C)
	  if(UpdateOffSet(newP, newC->ON, NULL, 0)) {
	      //ON(C) += SelectedMinterm - Perform Covering
	      if(UpdateOnSet(newC, SelectedMinterm, NULL, 0)) {
		  //If P has two children, additional updates are possible
		  if(newP->Inputs.size() == FaninBound) {
		      //ON(P) += {*OFF(I) : For all I in INPUT(P)}    Two inputs: ON(P) = OFF(C) * OFF(S)
		      BDD OffProd = ONE;
		      for(int i = 0; i < newP->Inputs.size(); i++) OffProd *= newP->Inputs[i]->OFF;
		      if(UpdateOnSet(newP, OffProd, NULL, 0)) {
			  //On(C) += OFF(P) * {*OFF(I) : For all I in INPUT(P), where I != C}    Two inputs: ON(C) += OFF(P) * OFF(S)
			  OffProd = newP->OFF;
			  for(int i = 0; i < newP->Inputs.size(); i++) if(newP->Inputs[i] != newC) OffProd *= newP->Inputs[i]->OFF;
			  if(UpdateOnSet(newC, OffProd, NULL, 0)) {
			      //ON(S) += OFF(P) * {*OFF(I) : For all I in INPUT(P), where I != S}  Two inputs: ON(S) += OFF(P) * OFF(C)
			      UpdateOK = true;
			      for(int i = 0; i < newP->Inputs.size() && UpdateOK; i++) {
				  Node* S = newP->Inputs[i];
				  if(S != C) {
				      OffProd = newP->OFF;
				      for(int j = 0; j < newP->Inputs.size(); j++)
					  if(newP->Inputs[j] != S) OffProd *= newP->Inputs[j]->OFF;
				      if(UpdateOnSet(S, OffProd, NULL, 0))
					  UpdateOK = true;
				      else { 	
					  if(print) cout<<"  Conflict updating on set of "<<S<<endl;
					  if(trace) status<<"  Conflict updating on set of "<<S<<endl;
					  UpdateOK = false;
				      }
				  }
			      }
			  } else {
			      if(print) cout<<"  Conflict updating on set of "<<newC<<endl;
			      if(trace) status<<"  Conflict updating on set of "<<newC<<endl;
			  }
		      } else {
			  if(print) cout<<"  Conflict updating on set of "<<newP<<endl;
			  if(trace) status<<"  Conflict updating on set of "<<newP<<endl;
		      }
		  } else UpdateOK = true;
	      } else {
		  if(print) cout<<"  Conflict updating on set of "<<newC<<endl;
		  if(trace) status<<"  Conflict updating on set of "<<newC<<endl;
	      }
	  } else {
	      if(print) cout<<"  Conflict updating off set of "<<newP<<endl;
	      if(trace) status<<"  Conflict updating off set of "<<newP<<endl;
	  }
      } else {
	  if(print) cout<<"  Conflict updating off set of "<<newC<<endl;
	  if(trace) status<<"  Conflict updating off set of "<<newC<<endl;
      }
  } else if(GateType == 3) {
      if(newP->LogicOp == 'N') {
	  if(UpdateOnSet(newP, newC->OFF, NULL, 0)) {
	      if(UpdateOffSet(newP, newC->ON, NULL, 0)) {
		  if(UpdateOnSet(newC, newP->OFF, NULL, 0)) {
		      if(UpdateOffSet(newC, newP->ON, NULL, 0)) {
			  UpdateOK = true;
		      } else {
			  if(print) cout<<"  Conflict updating off set of "<<newC<<endl;
			  if(trace) status<<"  Conflict updating off set of "<<newC<<endl;
		      }
		  } else {
		      if(print) cout<<"  Conflict updating on set of "<<newC<<endl;
		      if(trace) status<<"  Conflict updating on set of "<<newC<<endl;
		  }
	      } else {
		  if(print) cout<<"  Conflict updating off set of "<<newP<<endl;
		  if(trace) status<<"  Conflict updating off set of "<<newP<<endl;
	      }
	  } else {
	      if(print) cout<<"  Conflict updating on set of "<<newP<<endl;
	      if(trace) status<<"  Conflict updating on set of "<<newP<<endl;
	  }
      } else {
	  //And: On(C) <-  On(P)
	  //Or: Off(C) <- Off(P)
	  if((newP->LogicOp == 'A' && UpdateOnSet(newC, newP->ON, NULL, 0)) ||
	     (newP->LogicOp == 'O' && UpdateOffSet(newC, newP->OFF, NULL, 0))) {
	      //And: Off(P) <- Off(C)
	      //Or:   On(P) <-  On(C)
	      if((newP->LogicOp == 'A' && UpdateOffSet(newP, newC->OFF, NULL, 0)) ||
		 (newP->LogicOp == 'O' && UpdateOnSet(newP, newC->ON, NULL, 0))) {
		  //Make sure the selectedm Minterm is covered
		  //And: Off(C) <- SelectedMinterm
		  //Or:   On(C) <- SelectedMinterm
		  if((newP->LogicOp == 'A' && UpdateOffSet(newC, SelectedMinterm, NULL, 0)) ||
		     (newP->LogicOp == 'O' && UpdateOnSet(newC, SelectedMinterm, NULL, 0))) {
		      //If P has max allowed children, additional updates are possible
		      if(newP->Inputs.size() == FaninBound) {
			  //And: On(P) <-  On(I_1) & ... &  On(I_n)
			  //Or: Off(P) <- Off(I_1) & ... & Off(I_n)
			  BDD Product = ONE;
			  for(int i = 0; i < newP->Inputs.size(); i++)
			      if(newP->LogicOp == 'A') Product *= newP->Inputs[i]->ON;
			      else if(newP->LogicOp == 'O') Product *= newP->Inputs[i]->OFF;
			  if((newP->LogicOp == 'A' && UpdateOnSet(newP, Product, NULL, 0)) ||
			     (newP->LogicOp == 'O' && UpdateOffSet(newP, Product, NULL, 0))){
			      //And: Off(C) <- Off(P) &  On(I_1) & ... &  On(I_n)
			      //Or:   On(C) <-  On(P) & Off(I_1) & ... & Off(I_n)
			      if(newP->LogicOp == 'A') Product = newP->OFF;
			      else if(newP->LogicOp == 'O') Product = newP->ON;
			      for(int i = 0; i < newP->Inputs.size(); i++) 
				  if(newP->Inputs[i] != newC) 
				      if(newP->LogicOp == 'A') Product *= newP->Inputs[i]->ON;
				      else if(newP->LogicOp == 'O') Product *= newP->Inputs[i]->OFF;
			      if((newP->LogicOp == 'A' && UpdateOffSet(newC, Product, NULL, 0)) ||
				 (newP->LogicOp == 'O' && UpdateOnSet(newC, Product, NULL, 0))) {
				  UpdateOK = true;
				  //And: Off(S) <- Off(P) &  On(I_1) & ... &  On(I_n)
				  //Or:   On(S) <-  On(P) & Off(I_1) & ... & Off(I_n)
				  UpdateOK = true;
				  for(int i = 0; i < newP->Inputs.size() && UpdateOK; i++) {
				      Node* S = newP->Inputs[i];
				      if(S != C) {
					  if(newP->LogicOp == 'A') Product = newP->OFF;
					  else if(newP->LogicOp == 'O') Product = newP->ON;
					  for(int j = 0; j < newP->Inputs.size(); j++)
					      if(newP->Inputs[j] != S) {
						  if(newP->LogicOp == 'A') Product *= newP->Inputs[j]->ON;
						  else if(P->LogicOp == 'O') Product *= newP->Inputs[j]->OFF;
					      }
					  if((newP->LogicOp == 'A' && UpdateOffSet(S, Product, NULL, 0)) ||
					     (newP->LogicOp == 'O' && UpdateOnSet(S, Product, NULL, 0)))
					      UpdateOK = true;
					  else { 	
					      if(print) cout<<"  Conflict updating off set of "<<S<<endl;
					      if(trace) status<<"  Conflict updating off set of "<<S<<endl;
					      UpdateOK = false;
					  }
				      }
				  }
			      } else {
				  if(print) cout<<"  Conflict updating function of "<<newC<<endl;
				  if(trace) status<<"  Conflict updating function of "<<newC<<endl;
			      }
			  } else {
			      if(print) cout<<"  Conflict updating function of "<<newP<<endl;
			      if(trace) status<<"  Conflict updating function of "<<newP<<endl;
			  }
		      } else UpdateOK = true;
		  } else {
		      if(print) cout<<"  Conflict updating function of "<<newC<<endl;
		      if(trace) status<<"  Conflict updating function of "<<newC<<endl;
		  }
	      } else {
		  if(print) cout<<"  Conflict updating function of "<<newP<<endl;
		  if(trace) status<<"  Conflict updating function of "<<newP<<endl;
	      }
	  } else {
	      if(print) cout<<"  Conflict updating function of "<<newC<<endl;
	      if(trace) status<<"  Conflict updating function of "<<newC<<endl;
	  }
      }
  } else if(GateType == 4) {
      //And: On(C) <-  On(P)
      //Or: Off(C) <- Off(P)
      if((newP->LogicOp == 'A' && UpdateOnSet(newC, newP->ON, NULL, 0)) ||
	 (newP->LogicOp == 'O' && UpdateOffSet(newC, newP->OFF, NULL, 0))) {
	  //And: Off(P) <- Off(C)
	  //Or:   On(P) <-  On(C)
	  if((newP->LogicOp == 'A' && UpdateOffSet(newP, newC->OFF, NULL, 0)) ||
	     (newP->LogicOp == 'O' && UpdateOnSet(newP, newC->ON, NULL, 0))) {
	      //Make sure the selectedm Minterm is covered
	      //And: Off(C) <- SelectedMinterm
	      //Or:   On(C) <- SelectedMinterm
	      if((newP->LogicOp == 'A' && UpdateOffSet(newC, SelectedMinterm, NULL, 0)) ||
		 (newP->LogicOp == 'O' && UpdateOnSet(newC, SelectedMinterm, NULL, 0))) {
		  //If P has max allowed children, additional updates are possible
		  if(newP->Inputs.size() == FaninBound) {
		      //And: On(P) <-  On(I_1) & ... &  On(I_n)
		      //Or: Off(P) <- Off(I_1) & ... & Off(I_n)
		      BDD Product = ONE;
		      for(int i = 0; i < newP->Inputs.size(); i++)
			  if(newP->LogicOp == 'A') Product *= newP->Inputs[i]->ON;
			  else if(newP->LogicOp == 'O') Product *= newP->Inputs[i]->OFF;
		      if((newP->LogicOp == 'A' && UpdateOnSet(newP, Product, NULL, 0)) ||
			 (newP->LogicOp == 'O' && UpdateOffSet(newP, Product, NULL, 0))){
			  //And: Off(C) <- Off(P) &  On(I_1) & ... &  On(I_n)
			  //Or:   On(C) <-  On(P) & Off(I_1) & ... & Off(I_n)
			  if(newP->LogicOp == 'A') Product = newP->OFF;
			  else if(newP->LogicOp == 'O') Product = newP->ON;
			  for(int i = 0; i < newP->Inputs.size(); i++) 
			      if(newP->Inputs[i] != newC) 
				  if(newP->LogicOp == 'A') Product *= newP->Inputs[i]->ON;
				  else if(newP->LogicOp == 'O') Product *= newP->Inputs[i]->OFF;
			  if((newP->LogicOp == 'A' && UpdateOffSet(newC, Product, NULL, 0)) ||
			     (newP->LogicOp == 'O' && UpdateOnSet(newC, Product, NULL, 0))) {
			      UpdateOK = true;
			      //And: Off(S) <- Off(P) &  On(I_1) & ... &  On(I_n)
			      //Or:   On(S) <-  On(P) & Off(I_1) & ... & Off(I_n)
			      UpdateOK = true;
			      for(int i = 0; i < newP->Inputs.size() && UpdateOK; i++) {
				  Node* S = newP->Inputs[i];
				  if(S != C) {
				      if(newP->LogicOp == 'A') Product = newP->OFF;
				      else if(newP->LogicOp == 'O') Product = newP->ON;
				      for(int j = 0; j < newP->Inputs.size(); j++)
					  if(newP->Inputs[j] != S) {
					      if(newP->LogicOp == 'A') Product *= newP->Inputs[j]->ON;
					      else if(P->LogicOp == 'O') Product *= newP->Inputs[j]->OFF;
					  }
				      if((newP->LogicOp == 'A' && UpdateOffSet(S, Product, NULL, 0)) ||
					 (newP->LogicOp == 'O' && UpdateOnSet(S, Product, NULL, 0)))
					  UpdateOK = true;
				      else { 	
					  if(print) cout<<"  Conflict updating off set of "<<S<<endl;
					  if(trace) status<<"  Conflict updating off set of "<<S<<endl;
					  UpdateOK = false;
				      }
				  }
			      }
			  } else {
			      if(print) cout<<"  Conflict updating function of "<<newC<<endl;
			      if(trace) status<<"  Conflict updating function of "<<newC<<endl;
			  }
		      } else {
			  if(print) cout<<"  Conflict updating function of "<<newP<<endl;
			  if(trace) status<<"  Conflict updating function of "<<newP<<endl;
		      }
		  } else UpdateOK = true;
	      } else {
		  if(print) cout<<"  Conflict updating function of "<<newC<<endl;
		  if(trace) status<<"  Conflict updating function of "<<newC<<endl;
	      }
	  } else {
	      if(print) cout<<"  Conflict updating function of "<<newP<<endl;
	      if(trace) status<<"  Conflict updating function of "<<newP<<endl;
	  }
      } else {
	  if(print) cout<<"  Conflict updating function of "<<newC<<endl;
	  if(trace) status<<"  Conflict updating function of "<<newC<<endl;
      }
  } else if(GateType == 5) {
    //Nand: ON(C) += OFF(P)
    //Nor: OFF(C) += ON(P)
    if((newP->LogicOp == 'D' && UpdateOnSet(newC, newP->OFF, NULL, 0)) ||
       (newP->LogicOp == 'R' && UpdateOffSet(newC, newP->ON, NULL, 0))) {
      //Nand: ON(P) += OFF(C)
      //Nor:  ON(P) += ON(C)
      if((newP->LogicOp == 'D' && UpdateOnSet(newP, newC->OFF, NULL, 0)) ||
	 (newP->LogicOp == 'R' && UpdateOffSet(newP, newC->ON, NULL, 0))) {
	//Nand: OFF(C) += SelectedMinterm - Perform Covering
	//Nor:  ON(C) += SelectedMinterm - Perform Covering
	if((newP->LogicOp == 'D' && UpdateOffSet(newC, SelectedMinterm, NULL, 0)) ||
	   (newP->LogicOp == 'R' && UpdateOnSet(newC, SelectedMinterm, NULL, 0))) {
	  //If P has two children, additional updates are possible
	  if(newP->Inputs.size() == FaninBound) {
	    //Nand: OFF(P) += {*ON(I) : For all I in INPUT(P)}    Two inputs: OFF(P) = ON(C) * ON(S)
	    //Nor:  ON(P) += {*OFF(I) : For all In in INPUT(P)}  Two inputs: ON(P) = OFF(C) * OFF(S)
	    BDD Product = ONE;
	    for(int i = 0; i < newP->Inputs.size(); i++) 
	      if(newP->LogicOp == 'D') Product *= newP->Inputs[i]->ON;
	      else if(newP->LogicOp == 'R') Product *= newP->Inputs[i]->OFF;
	    if((newP->LogicOp == 'D' && UpdateOffSet(newP, Product, NULL, 0)) ||
	       (newP->LogicOp == 'R' && UpdateOnSet(newP, Product, NULL, 0))) {
	      //Nand: OFF(C) += ON(P) * {*ON(I) : For all I in INPUT(P), where I != C}    Two inputs: OFF(C) += ON(P) * ON(S)
	      //Nor:  ON(C) += OFF(P) * {*OFF(I) : For all I in INPUT(P), where I != C}   Two inputs: ON(C) += OFF(P) * OFF(S)
	      if(newP->LogicOp == 'D') Product = newP->ON;
	      else if(newP->LogicOp == 'R') Product = newP->OFF;
	      for(int i = 0; i < newP->Inputs.size(); i++) 
		if(newP->Inputs[i] != newC) 
		  if(newP->LogicOp == 'D') Product *= newP->Inputs[i]->ON;
		  else if(newP->LogicOp == 'R') Product *= newP->Inputs[i]->OFF;
	      if((newP->LogicOp == 'D' && UpdateOffSet(newC, Product, NULL, 0)) ||
		 (newP->LogicOp == 'R' && UpdateOnSet(newC, Product, NULL, 0))) {
		//Nand: OFF(S) += ON(P) * {*ON(I) : For all I in INPUT(P), where I != S}  Two inputs: OFF(S) += ON(P) * ON(C)
		//Nor:  ON(S) += OFF(P) * {*OFF(I) : For all I in INPUT(P), where I != S} Two inputs: ON(S) += OFF(P) * OFF(C)
		UpdateOK = true;
		for(int i = 0; i < newP->Inputs.size() && UpdateOK; i++) {
		  Node* S = newP->Inputs[i];
		  if(S != C) {
		    if(newP->LogicOp == 'D') Product = newP->ON;
		    else if(newP->LogicOp == 'N') Product = newP->OFF;
		    for(int j = 0; j < newP->Inputs.size(); j++)
		      if(newP->Inputs[j] != S) 
			if(newP->LogicOp == 'D') Product *= newP->Inputs[j]->ON;
			else if(newP->LogicOp == 'R') Product *= newP->Inputs[j]->OFF;
		    if((newP->LogicOp == 'D' && UpdateOffSet(S, Product, NULL, 0)) ||
		       (newP->LogicOp == 'R' && UpdateOnSet(S, Product, NULL, 0)))
		      UpdateOK = true;
		    else { 	
		      if(print) cout<<"  Conflict updating "<<S<<endl;
		      if(trace) status<<"  Conflict updating "<<S<<endl;
		      UpdateOK = false;
		    }
		  }
		}
	      } else {
		if(print) cout<<"  Conflict updating "<<newC<<endl;
		if(trace) status<<"  Conflict updating "<<newC<<endl;
	      }
	    } else {
	      if(print) cout<<"  Conflict updating "<<newP<<endl;
	      if(trace) status<<"  Conflict updating "<<newP<<endl;
	    }
	  } else UpdateOK = true;
	} else {
	  if(print) cout<<"  Conflict updating "<<newC<<endl;
	  if(trace) status<<"  Conflict updating "<<newC<<endl;
	}
      } else {
	if(print) cout<<"  Conflict updating "<<newP<<endl;
	if(trace) status<<"  Conflict updating "<<newP<<endl;
      }
    } else {
      if(print) cout<<"  Conflict updating "<<newC<<endl;
      if(trace) status<<"  Conflict updating "<<newC<<endl;
    }
  }
  if(!UpdateOK) {
    CONFLICTCOUNT++;
      delete newNet;
      return NULL;
  }
  
  newNet->UpdateConnectibleSets();
  if(SimpleBridge) {
      //Tries to find on/off-set updates within simple bridges.
      if(FaninBound == 2 && !newNet->SimpleBridgeImpl(newC, newP)) {
	  CONFLICTCOUNT++;
	  delete newNet;
	  return NULL;
      }
      //Connectible Set updated in UpdateBridges function
  }
  
  if(ExtendedBridgeC) {
      //Tries to find on/off-set updates within bridges that have a base of C.
      if(FaninBound == 2 && !newNet->ExtendedBridgeImpl(newC, newP, true)) {
	  CONFLICTCOUNT++;
	  delete newNet;
	  return NULL;
      }
      newNet->UpdateConnectibleSets();
  } else if(ExtendedBridgeAll) {
      //Tries to find on/off-set updates within extended bridges
      if(FaninBound == 2 && !newNet->ExtendedBridgeImpl(newC, newP, false)) {
	  CONFLICTCOUNT++;
	  delete newNet;
	  return NULL;
      }
      newNet->UpdateConnectibleSets();
  }
  
  return newNet;
}

//Tries to find on/off-set updates within simple bridges.  Simple bridges are subnetworks of the form: 
//     L 
//   /   \
//  N     G
//   \   /
//     R 
bool Network::SimpleBridgeImpl(Node* C, Node* P) {
    if(FaninBound != 2) return true;
    bool changes = true;
    while(changes) {
	changes = false;
	//Search all outputs for a bridge
	for(int i = 0; i < Outputs.size(); i++) {
	    Node* N = Outputs[i];
	    //N must have two outputs and some uncovered minterms for a bridge update to occur
	    if(N->Uncovered != ZERO && N->Inputs.size() == 2) {
		//Bridge exists if N's two inputs share an input.
		Node* L = N->Inputs[0];
		Node* R = N->Inputs[1];
		for(int l = 0; l < L->Inputs.size(); l++) {
		    for(int r = 0; r < R->Inputs.size(); r++) {
			if(L->Inputs[l] == R->Inputs[r]) {
			    Node* G = L->Inputs[l];
			    if(print) cout<<"Found Bridge: "<<N<<" -> ("<<L<<", "<<R<<") -> "<<G<<endl;
			    BDD Implications = N->Uncovered;
			    if(GateType == 1) {
				BDD PrevGON = G->ON;
				if(!UpdateOnSet(G, Implications, NULL, 0)) 
				    return false;
				if(PrevGON != G->ON) {
				    SimpleBridgeCount++;
				    changes = true;
				}
			    } else if(GateType == 2) {
				BDD PrevGOFF = G->OFF;
				if(!UpdateOffSet(G, Implications, NULL, 0)) 
				    return false;
				if(PrevGOFF != G->OFF) {
				    SimpleBridgeCount++;
				    changes = true;
				}
			    } else if(GateType == 3) {
				if(N->LogicOp == 'A' && L->LogicOp == 'O' && R->LogicOp == 'O') {
				    BDD PrevGOFF = G->OFF;
				    if(!UpdateOffSet(G, Implications, NULL, 0))
					return false;
				    if(PrevGOFF != G->OFF) {
					SimpleBridgeCount++;
					changes = true;
				    }
				} else if(G->LogicOp == 'O' && L->LogicOp == 'A' && R->LogicOp == 'A') {
				    BDD PrevGON = G->ON;
				    if(!UpdateOnSet(G, Implications, NULL, 0))
					return false;
				    if(PrevGON != G->ON) {
					SimpleBridgeCount++;
					changes = true;
				    }
				}
			    } else if(GateType == 4) {
				if(N->LogicOp == 'A' && L->LogicOp == 'O' && R->LogicOp == 'O') {
				    BDD PrevGOFF = G->OFF;
				    if(!UpdateOffSet(G, Implications, NULL, 0))
					return false;
				    if(PrevGOFF != G->OFF) {
					SimpleBridgeCount++;
					changes = true;
				    }
				} else if(G->LogicOp == 'O' && L->LogicOp == 'A' && R->LogicOp == 'A') {
				    BDD PrevGON = G->ON;
				    if(!UpdateOnSet(G, Implications, NULL, 0))
					return false;
				    if(PrevGON != G->ON) {
					SimpleBridgeCount++;
					changes = true;
				    }
				}
			    } 
			}
		    }
		}
	    }
	}
	//Search all internal nodes for a bridge
	for(int i = 0; i < Internal.size(); i++) {
	    Node* N = Internal[i];
	    //N must have two outputs and some uncovered minterms for a bridge update to occur
	    if(N->Uncovered != ZERO && N->Inputs.size() == 2) {
		//Bridge exists if N's two inputs share an input.
		Node* L = N->Inputs[0];
		Node* R = N->Inputs[1];
		for(int l = 0; l < L->Inputs.size(); l++) {
		    for(int r = 0; r < R->Inputs.size(); r++) {
			if(L->Inputs[l] == R->Inputs[r]) {
			    Node* G = L->Inputs[l];
			    if(print) cout<<"Found Bridge: "<<N<<" -> ("<<L<<", "<<R<<") -> "<<G<<endl;
			    BDD Implications = N->Uncovered;
			    if(GateType == 1) {
				BDD PrevGON = G->ON;
				if(!UpdateOnSet(G, Implications, NULL, 0)) 
				    return false;
				if(PrevGON != G->ON) {
				    SimpleBridgeCount++;
				    changes = true;
				}
			    } else if(GateType == 2) {
				BDD PrevGOFF = G->OFF;
				if(!UpdateOffSet(G, Implications, NULL, 0)) 
				    return false;
				if(PrevGOFF != G->OFF) {
				    SimpleBridgeCount++;
				    changes = true;
				}
			    } else if(GateType == 3) {
				if(N->LogicOp == 'A' && L->LogicOp == 'O' && R->LogicOp == 'O') {
				    BDD PrevGOFF = G->OFF;
				    if(!UpdateOffSet(G, Implications, NULL, 0))
					return false;
				    if(PrevGOFF != G->OFF) {
					SimpleBridgeCount++;
					changes = true;
				    }
				} else if(G->LogicOp == 'O' && L->LogicOp == 'A' && R->LogicOp == 'A') {
				    BDD PrevGON = G->ON;
				    if(!UpdateOnSet(G, Implications, NULL, 0))
					return false;
				    if(PrevGON != G->ON) {
					SimpleBridgeCount++;
					changes = true;
				    }
				}
			    } else if(GateType == 4) {
				if(N->LogicOp == 'A' && L->LogicOp == 'O' && R->LogicOp == 'O') {
				    BDD PrevGOFF = G->OFF;
				    if(!UpdateOffSet(G, Implications, NULL, 0))
					return false;
				    if(PrevGOFF != G->OFF) {
					SimpleBridgeCount++;
					changes = true;
				    }
				} else if(G->LogicOp == 'O' && L->LogicOp == 'A' && R->LogicOp == 'A') {
				    BDD PrevGON = G->ON;
				    if(!UpdateOnSet(G, Implications, NULL, 0))
					return false;
				    if(PrevGON != G->ON) {
					SimpleBridgeCount++;
					changes = true;
				    }
				}
			    }
			}
		    }
		}
	    }
	}
	if(changes) {
	    //PrintPicture();
	    //cin.get();
	    UpdateConnectibleSets();
	}
    }
    return true;
}



//Tries to find on/off-set updates within bridges.  Bridges are subnetworks of the form: 
//      N1 - - - -
//    /            \
//  N               C
//    \            /
//      N2 - - - - 

bool Network::ExtendedBridgeImpl(Node* C, Node* P, bool version) {
    if(version) {
	//If C has more than one parent then it's potentially the base of a bridge
	if(C->Parents.size() > 1) {
	    set<Node*> Ancestors;
	    set<Node*> Intersection;
	    set<Node*> OnlyOne;
	    //Collect all the ancestors of C.  Find those that are ancestors by different parents.
	    for(set<Node*>::iterator i = C->Parents.begin(); i != C->Parents.end(); i++) {
		Ancestors = GetAncestors(*i);
		if(i == C->Parents.begin()) OnlyOne = Ancestors;
		else
		    for(set<Node*>::iterator j = Ancestors.begin(); j != Ancestors.end(); j++) {
			if(OnlyOne.find(*j) != OnlyOne.end()) {
			    Intersection.insert(*j);
			    OnlyOne.erase(*j);
			} else if(Intersection.find(*j) == Intersection.end())
			    OnlyOne.insert(*j);
		    }
	    }
	    //If C has an ancestor by different parents, then there is a bridge in Network.
	    //Iterate through these bridges to look for additional on/off-set updates
	    for(set<Node*>::iterator k = Intersection.begin(); k != Intersection.end(); k++) {
		//Additional updates can only be made in a bridge if the root of the bridge has max fan-in
		if((*k)->Inputs.size() == FaninBound) {
		    //cout<<"Bridge: "<<*k<<" -> "<<C<<endl;
		    BDD UncoveredMinterms = (*k)->Uncovered;
		    if(UncoveredMinterms != ZERO) {
			//cout<<"  Uncovered Minterms: "; PrintFunction(UncoveredMinterms); cout<<endl;
			bool Cont = true;
			//On and off-sets for various ways of covering uncovered minterms
			vector<map<Node*,BDD> > AllOn((*k)->Inputs.size());
			vector<map<Node*,BDD> > AllOff((*k)->Inputs.size());
			for(int i = 0; i < (*k)->Inputs.size() && Cont; i++) {
			    Node* I = (*k)->Inputs[i]; 
			    //cout<<"  Cover with "<<I<<endl;
			    if(GateType == 1) {
				if(!UpdateOffMap(I, UncoveredMinterms, AllOn[i], AllOff[i])) {
				    Cont = false;
				    //cout<<"    Cover didn't work"<<endl;
				}
			    } else if(GateType == 2) {
				if(!UpdateOnMap(I, UncoveredMinterms, AllOn[i], AllOff[i])) {
				    Cont = false;
				}
			    } else if(GateType == 3 || GateType == 4) {
				if(((*k)->LogicOp == 'A' && !UpdateOffMap(I, UncoveredMinterms, AllOn[i], AllOff[i])) ||
				   ((*k)->LogicOp == 'O' && !UpdateOnMap(I, UncoveredMinterms, AllOn[i], AllOff[i])))
				    Cont = false;
			    }
			}
			if(Cont) {
			    //All Updates worked.
			    for(int i = 0; i < Internal.size(); i++) {
				bool ONFound = true;
				bool OFFFound = true;
				BDD ON = ONE;
				BDD OFF = ONE;
				for(int j = 0; j < AllOn.size() && (ONFound || OFFFound); j++) {
				    if(AllOn[j].find(Internal[i]) != AllOn[j].end())
					ON *= AllOn[j][Internal[i]];
				    else ONFound = false;
				    if(AllOff[j].find(Internal[i]) != AllOff[j].end())
					OFF *= AllOff[j][Internal[i]];
				    else OFFFound = false;
				}
				if(ON != Internal[i]->ON && ONFound) {
				    ExtendedBridgeCount++;
				    if(print) {
					//PrintPicture();
					cout<<"  Bridge (1) Implication "<<Internal[i]<<"->ON = "; PrintFunction(ON * !Internal[i]->ON); cout<<endl;
					//cin.get();
				    }
				    if(!UpdateOnSet(Internal[i], ON, NULL, 0)) {cin.get(); return false;}
				}
				if(OFF != Internal[i]->OFF && OFFFound) {
				    ExtendedBridgeCount++;
				    if(print) {
					//PrintPicture();
					cout<<"  Bridge (1) Implication "<<Internal[i]<<"->OFF = "; PrintFunction(OFF * !Internal[i]->OFF); cout<<endl;
					//cin.get();
				    }
				    if(!UpdateOffSet(Internal[i], OFF, NULL, 0)) { cin.get(); return false;}
				}
			    }
			    //What to do if all updates didn't work?
			} else {
			}
		    }
		}
	    }
	}
	return true;
    } else {
	for(int i = 0; i < Outputs.size(); i++) {
	    Node* N = Outputs[i];
	    if(N->Inputs.size() == FaninBound && N->Uncovered != ZERO) {
		set<Node*> CommonDescendents = FindAllDescendents(N->Inputs[0]);
		for(int j = 1; j < N->Inputs.size() && !CommonDescendents.empty(); j++) {
		    set<Node*> Descendents = FindAllDescendents(N->Inputs[j]);
		    set<Node*> NewCommon;
		    for(set<Node*>::iterator c = CommonDescendents.begin(); c != CommonDescendents.end(); c++)
			if(Descendents.find(*c) != Descendents.end())
			    NewCommon.insert(*c);
		    CommonDescendents = NewCommon;
		}
		if(!CommonDescendents.empty()) {
		    BDD UncoveredMinterms = N->Uncovered;
		    //cout<<"Bridge: "<<N<<" -> {";
		    //for(set<Node*>::iterator c = CommonDescendents.begin(); c != CommonDescendents.end(); c++) {
		    //if(c != CommonDescendents.begin()) cout<<", ";
		    //cout<<*c;
		    //}
		    //cout<<"}"<<endl;
		    bool Cont = true;
		    //On and off-sets for various ways of covering uncovered minterms
		    vector<map<Node*,BDD> > AllOn(N->Inputs.size());
		    vector<map<Node*,BDD> > AllOff(N->Inputs.size());
		    for(int j = 0; j < N->Inputs.size() && Cont; j++) {
			Node* I = N->Inputs[j]; 
			//cout<<"  Cover with "<<I<<endl;
			if(GateType == 1) {
			    if(!UpdateOffMap(I, UncoveredMinterms, AllOn[j], AllOff[j])) {
				Cont = false;
				//cout<<"    Cover didn't work"<<endl;
			    }
			} else if(GateType == 2) {
			    if(!UpdateOnMap(I, UncoveredMinterms, AllOn[j], AllOff[j])) {
				Cont = false;
			    }
			} else if(GateType == 3 || GateType == 4) {
			    if((N->LogicOp == 'A' && !UpdateOffMap(I, UncoveredMinterms, AllOn[j], AllOff[j])) ||
			       (N->LogicOp == 'O' && !UpdateOnMap(I, UncoveredMinterms, AllOn[j], AllOff[j])))
				Cont = false;
			}
		    }
		    if(Cont) {
			//All Updates worked.
			for(int j = 0; j < Internal.size(); j++) {
			    Node* I = Internal[j];
			    bool ONFound = true;  bool OFFFound = true;
			    BDD ON = ONE;         BDD OFF = ONE;
			    for(int k = 0; k < AllOn.size() && (ONFound || OFFFound); k++) {
				if(AllOn[k].find(I) != AllOn[k].end())
				    ON *= AllOn[k][I];
				else ONFound = false;
				if(AllOff[k].find(I) != AllOff[k].end())
				    OFF *= AllOff[k][I];
				else OFFFound = false;
			    }
			    if(ON != I->ON && ONFound) {
				ExtendedBridgeCount++;
				if(print) {
				    //PrintPicture();
				    cout<<"  Extended Bridge Implication "<<I<<"->ON = "; PrintFunction(ON * !I->ON); cout<<endl;
				    //cin.get();
				}
				if(!UpdateOnSet(I, ON, NULL, 0)) {cin.get(); return false;}
			    }
			    if(OFF != I->OFF && OFFFound) {
				ExtendedBridgeCount++;
				if(print) {
				    //PrintPicture();
				    cout<<"  Extended Bridge Implication "<<I<<"->OFF = "; PrintFunction(OFF * !I->OFF); cout<<endl;
				    //cin.get();
				}
				if(!UpdateOffSet(I, OFF, NULL, 0)) { cin.get(); return false;}
			    }
			}
			//What to do if all updates didn't work?
		    } else {
		    }
		}
	    }
	}
	for(int i = 0; i < Internal.size(); i++) {
	    Node* N = Internal[i];
	    if(N->Inputs.size() == FaninBound && N->Uncovered != ZERO) {
		set<Node*> CommonDescendents = FindAllDescendents(N->Inputs[0]);
		for(int j = 1; j < N->Inputs.size() && !CommonDescendents.empty(); j++) {
		    set<Node*> Descendents = FindAllDescendents(N->Inputs[j]);
		    set<Node*> NewCommon;
		    for(set<Node*>::iterator c = CommonDescendents.begin(); c != CommonDescendents.end(); c++)
			if(Descendents.find(*c) != Descendents.end())
			    NewCommon.insert(*c);
		    CommonDescendents = NewCommon;
		}
		if(!CommonDescendents.empty()) {
		    BDD UncoveredMinterms = N->Uncovered;
		    //cout<<"Bridge: "<<N<<" -> {";
		    //for(set<Node*>::iterator c = CommonDescendents.begin(); c != CommonDescendents.end(); c++) {
		    //if(c != CommonDescendents.begin()) cout<<", ";
		    //cout<<*c;
		    //}
		    //cout<<"}"<<endl;
		    bool Cont = true;
		    //On and off-sets for various ways of covering uncovered minterms
		    vector<map<Node*,BDD> > AllOn(N->Inputs.size());
		    vector<map<Node*,BDD> > AllOff(N->Inputs.size());
		    for(int j = 0; j < N->Inputs.size() && Cont; j++) {
			Node* I = N->Inputs[j]; 
			//cout<<"  Cover with "<<I<<endl;
			if(GateType == 1) {
			    if(!UpdateOffMap(I, UncoveredMinterms, AllOn[j], AllOff[j])) {
				Cont = false;
				//cout<<"    Cover didn't work"<<endl;
			    }
			} else if(GateType == 2) {
			    if(!UpdateOnMap(I, UncoveredMinterms, AllOn[j], AllOff[j])) {
				Cont = false;
			    }
			} else if(GateType == 3 || GateType == 4) {
			    if((N->LogicOp == 'A' && !UpdateOffMap(I, UncoveredMinterms, AllOn[j], AllOff[j])) ||
			       (N->LogicOp == 'O' && !UpdateOnMap(I, UncoveredMinterms, AllOn[j], AllOff[j])))
				Cont = false;
			}
		    }
		    if(Cont) {
			//All Updates worked.
			for(int j = 0; j < Internal.size(); j++) {
			    Node* I = Internal[j];
			    bool ONFound = true;  bool OFFFound = true;
			    BDD ON = ONE;         BDD OFF = ONE;
			    for(int k = 0; k < AllOn.size() && (ONFound || OFFFound); k++) {
				if(AllOn[k].find(I) != AllOn[k].end())
				    ON *= AllOn[k][I];
				else ONFound = false;
				if(AllOff[k].find(I) != AllOff[k].end())
				    OFF *= AllOff[k][I];
				else OFFFound = false;
			    }
			    if(ON != I->ON && ONFound) {
				ExtendedBridgeCount++;
				if(print) {
				    //PrintPicture();
				    cout<<"  Extended Bridge Implication "<<I<<"->ON = "; PrintFunction(ON * !I->ON); cout<<endl;
				    //cin.get();
				}
				if(!UpdateOnSet(I, ON, NULL, 0)) {cin.get(); return false;}
			    }
			    if(OFF != I->OFF && OFFFound) {
				ExtendedBridgeCount++;
				if(print) {
				    //PrintPicture();
				    cout<<"  Extended Bridge Implication "<<I<<"->OFF = "; PrintFunction(OFF * !I->OFF); cout<<endl;
				    //cin.get();
				}
				if(!UpdateOffSet(I, OFF, NULL, 0)) { cin.get(); return false;}
			    }
			}
			//What to do if all updates didn't work?
		    } else {
		    }
		}
		
	    }
	}
    }
}

bool UpdateOnMap(Node* N, BDD OnSet, map<Node*, BDD>& OnMap, map<Node*, BDD>& OffMap) {
  if(OnMap.find(N) == OnMap.end())
      OnMap[N] = N->ON;
  if(OffMap.find(N) == OffMap.end())
      OffMap[N] = N->OFF;
  BDD oldON = OnMap[N];
  OnMap[N] += OnSet;

  if(OnMap[N] * OffMap[N] != ZERO || OnMap[N] == ONE || OffMap[N] == ONE)
      return false;

  if(oldON != OnMap[N]) {
      if(GateType == 1) {
	  //Nand:   OFF(P) <- ON(N) * ON(S)     ON(P) <- OFF(N)
	  //        OFF(S) <- ON(N) * ON(P)     
	  //      OFF(I_1) <- ON(N) * ON(I_2) ON(I_1) <- OFF(N)
	  for(set<Node*>::iterator i = N->Parents.begin(); i != N->Parents.end(); i++) {
	      Node* P = *i;
	      if(P->Inputs.size() == FaninBound) {
		  for(int j = 0; j < P->Inputs.size(); j++) {
		      Node* S = P->Inputs[j];
		      if(S != N) {
			  if(OnMap.find(S) == OnMap.end())
			      OnMap[S] = S->ON;
			  if(OnMap.find(P) == OnMap.end())
			      OnMap[P] = P->ON;
			  if(S != N) {
			      if(!UpdateOffMap(P, OnMap[N] * OnMap[S], OnMap, OffMap)) return false;
			      if(!UpdateOffMap(S, OnMap[N] * OnMap[P], OnMap, OffMap)) return false;
			  }
		      }
		  }
	      }
	  }
	  if(N->Inputs.size() == FaninBound) {
	      for(int i = 0; i < N->Inputs.size(); i++) {
		  BDD Cover = OnMap[N];
		  for(int j = 0; j < N->Inputs.size(); j++) {
		      if(N->Inputs[j] != N->Inputs[i]) {
			  if(OnMap.find(N->Inputs[j]) == OnMap.end())
			      OnMap[N->Inputs[j]] = N->Inputs[j]->ON;
			  Cover *= OnMap[N->Inputs[j]];
		      }
		  }
		  if(Cover != ZERO)
		      if(!UpdateOffMap(N->Inputs[i], Cover, OnMap, OffMap)) return false;
	      }
	  }
      } else if(GateType == 2) {
	  //Nor:   ON(P) <- OFF(N) * OFF(S)     OFF(P) <- ON(N)
	  //        ON(S) <- OFF(N) * OFF(P)     
	  //      ON(I_1) <- OFF(N) * OFF(I_2) OFF(I_1) <- ON(N)
	  for(set<Node*>::iterator i = N->Parents.begin(); i != N->Parents.end(); i++) {
	      if(!UpdateOffMap(*i, OnMap[N], OnMap, OffMap)) return false;
	  }
	  for(int i = 0; i < N->Inputs.size(); i++)
	      if(!UpdateOffMap(N->Inputs[i], OnMap[N], OnMap, OffMap)) return false;
	  
      } else if(GateType == 3) {
	  //And(P):  ON(P) <- ON(N) * ON(S)       OFF(P) <- OFF(N)
	  //        OFF(S) <- ON(N) * OFF(P) 
	  //Or(P):   ON(P) <- ON(N)               OFF(P) <- OFF(N) * OFF(S) 
	  //                                       ON(S) <- OFF(N) * ON(P)
	  //Not(P):  OFF(P) <- ON(N)               ON(P) <- OFF(N)          
	  //And(N): ON(I_1) <- ON(N)            OFF(I_1) <- OFF(N) * ON(I_2)
	  //Or(N):  ON(I_1) <- ON(N) * OFF(I_2) OFF(I_1) <- OFF(N)
	  //Not(N):  OFF(I) <- ON(N)               ON(I) <- OFF(N)
	  
	  for(set<Node*>::iterator i = N->Parents.begin(); i != N->Parents.end(); i++) {
	      Node* P = *i;
	      if(P->LogicOp == 'O') {
		  if(!UpdateOnMap(P, OnMap[N], OnMap, OffMap)) return false;
	      } else if(P->LogicOp == 'N') {
		  if(!UpdateOffMap(P, OnMap[N], OnMap, OffMap)) return false;
	      } else if(P->LogicOp == 'A') {
		  if(P->Inputs.size() == FaninBound) {
		      for(int j = 0; j < P->Inputs.size(); j++) {
			  Node* S = P->Inputs[j];
			  if(S != N) {
			      if(OnMap.find(S) == OnMap.end())
				  OnMap[S] = S->ON;
			      if(OffMap.find(P) == OffMap.end())
				  OffMap[P] = P->OFF;
			      if(S != N) {
				  if(!UpdateOnMap(P, OnMap[N] * OnMap[S], OnMap, OffMap)) return false;
				  if(!UpdateOffMap(S, OnMap[N] * OffMap[P], OnMap, OffMap)) return false;
			      }
			  }
		      }
		  }
	      }
	  }
	  if(N->LogicOp == 'O') {
	      if(N->Inputs.size() == FaninBound) {
		  for(int i = 0; i < N->Inputs.size(); i++) {
		      BDD Cover = OnMap[N];
		      for(int j = 0; j < N->Inputs.size(); j++) {
			  if(N->Inputs[j] != N->Inputs[i]) {
			      if(OffMap.find(N->Inputs[j]) == OffMap.end())
				  OffMap[N->Inputs[j]] = N->Inputs[j]->OFF;
			      Cover *= OffMap[N->Inputs[j]];
			  }
		      }
		      if(Cover != ZERO)
			  if(!UpdateOnMap(N->Inputs[i], Cover, OnMap, OffMap)) return false;
		  }
	      }
	  } else if(N->LogicOp == 'N') {
	      if(!N->Inputs.empty()) if(!UpdateOffMap(N->Inputs[0], OnMap[N], OnMap, OffMap)) return false;
	  } else if(N->LogicOp == 'A') {
	      for(int i = 0; i < N->Inputs.size(); i++)
		  if(!UpdateOnMap(N->Inputs[i], OnMap[N], OnMap, OffMap)) return false;
	  }
     } else if(GateType == 4) {
	  //And(P):  ON(P) <- ON(N) * ON(S)       OFF(P) <- OFF(N)
	  //        OFF(S) <- ON(N) * OFF(P) 
	  //Or(P):   ON(P) <- ON(N)               OFF(P) <- OFF(N) * OFF(S) 
	  //                                       ON(S) <- OFF(N) * ON(P)
	  //And(N): ON(I_1) <- ON(N)            OFF(I_1) <- OFF(N) * ON(I_2)
	  //Or(N):  ON(I_1) <- ON(N) * OFF(I_2) OFF(I_1) <- OFF(N)
	  
	  for(set<Node*>::iterator i = N->Parents.begin(); i != N->Parents.end(); i++) {
	      Node* P = *i;
	      if(P->LogicOp == 'O') {
		  if(!UpdateOnMap(P, OnMap[N], OnMap, OffMap)) return false;
	      } else if(P->LogicOp == 'A') {
		  if(P->Inputs.size() == FaninBound) {
		      for(int j = 0; j < P->Inputs.size(); j++) {
			  Node* S = P->Inputs[j];
			  if(S != N) {
			      if(OnMap.find(S) == OnMap.end())
				  OnMap[S] = S->ON;
			      if(OffMap.find(P) == OffMap.end())
				  OffMap[P] = P->OFF;
			      if(S != N) {
				  if(!UpdateOnMap(P, OnMap[N] * OnMap[S], OnMap, OffMap)) return false;
				  if(!UpdateOffMap(S, OnMap[N] * OffMap[P], OnMap, OffMap)) return false;
			      }
			  }
		      }
		  }
	      }
	  }
	  if(N->LogicOp == 'O') {
	      if(N->Inputs.size() == FaninBound) {
		  for(int i = 0; i < N->Inputs.size(); i++) {
		      BDD Cover = OnMap[N];
		      for(int j = 0; j < N->Inputs.size(); j++) {
			  if(N->Inputs[j] != N->Inputs[i]) {
			      if(OffMap.find(N->Inputs[j]) == OffMap.end())
				  OffMap[N->Inputs[j]] = N->Inputs[j]->OFF;
			      Cover *= OffMap[N->Inputs[j]];
			  }
		      }
		      if(Cover != ZERO)
			  if(!UpdateOnMap(N->Inputs[i], Cover, OnMap, OffMap)) return false;
		  }
	      }
	  } else if(N->LogicOp == 'A') {
	      for(int i = 0; i < N->Inputs.size(); i++)
		  if(!UpdateOnMap(N->Inputs[i], OnMap[N], OnMap, OffMap)) return false;
	  }
      }
  }
  return true;
}

bool UpdateOffMap(Node* N, BDD OffSet, map<Node*, BDD>& OnMap, map<Node*, BDD>& OffMap) {
  if(OnMap.find(N) == OnMap.end())
      OnMap[N] = N->ON;
  if(OffMap.find(N) == OffMap.end())
      OffMap[N] = N->OFF;
  BDD oldOFF = OffMap[N];
  OffMap[N] += OffSet;

  if(OnMap[N] * OffMap[N] != ZERO || OnMap[N] == ONE || OffMap[N] == ONE)
      return false;
  
  if(oldOFF != OffMap[N]) {
      if(GateType == 1) { 
	  for(set<Node*>::iterator i = N->Parents.begin(); i != N->Parents.end(); i++) {
	      if(!UpdateOnMap(*i, OffMap[N], OnMap, OffMap)) return false;
	  }
	  for(int i = 0; i < N->Inputs.size(); i++)
	      if(!UpdateOnMap(N->Inputs[i], OffMap[N], OnMap, OffMap)) return false;
      } else if(GateType == 2) {
	  for(set<Node*>::iterator i = N->Parents.begin(); i != N->Parents.end(); i++) {
	      Node* P = *i;
	      if(P->Inputs.size() == FaninBound) {
		  for(int j = 0; j < P->Inputs.size(); j++) {
		      Node* S = P->Inputs[j];
		      if(S != N) {
			  if(OffMap.find(S) == OffMap.end())
			      OffMap[S] = S->OFF;
			  if(OffMap.find(P) == OffMap.end())
			      OffMap[P] = P->OFF;
			  if(S != N) {
			      if(!UpdateOnMap(P, OffMap[N] * OffMap[S], OnMap, OffMap)) return false;
			      if(!UpdateOnMap(S, OffMap[N] * OffMap[P], OnMap, OffMap)) return false;
			  }
		      }
		  }
	      }
	  }
	  if(N->Inputs.size() == FaninBound) {
	      for(int i = 0; i < N->Inputs.size(); i++) {
		  BDD Cover = OffMap[N];
		  for(int j = 0; j < N->Inputs.size(); j++) {
		      if(N->Inputs[j] != N->Inputs[i]) {
			  if(OffMap.find(N->Inputs[j]) == OffMap.end())
			      OffMap[N->Inputs[j]] = N->Inputs[j]->OFF;
			  Cover *= OffMap[N->Inputs[j]];
		      }
		  }
		  if(Cover != ZERO)
		      if(!UpdateOnMap(N->Inputs[i], Cover, OnMap, OffMap)) return false;
	      }
	  }
      } else if(GateType == 3) {
	  //And(P):  ON(P) <- ON(N) * ON(S)       OFF(P) <- OFF(N)
	  //        OFF(S) <- ON(N) * OFF(P) 
	  //Or(P):   ON(P) <- ON(N)               OFF(P) <- OFF(N) * OFF(S) 
	  //                                       ON(S) <- OFF(N) * ON(P)
	  //Not(P):  OFF(P) <- ON(N)               ON(P) <- OFF(N)          
	  //And(N): ON(I_1) <- ON(N)            OFF(I_1) <- OFF(N) * ON(I_2)
	  //Or(N):  ON(I_1) <- ON(N) * OFF(I_2) OFF(I_1) <- OFF(N)
	  //Not(N):  OFF(I) <- ON(N)               ON(I) <- OFF(N)
	  
	  for(set<Node*>::iterator i = N->Parents.begin(); i != N->Parents.end(); i++) {
	      Node* P = *i;
	      if(P->LogicOp == 'A') {
		  if(!UpdateOffMap(P, OffMap[N], OnMap, OffMap)) return false;
	      } else if(P->LogicOp == 'N') {
		  if(!UpdateOnMap(P, OffMap[N], OnMap, OffMap)) return false;
	      } else if(P->LogicOp == 'O') {
		  if(P->Inputs.size() == FaninBound) {
		      for(int j = 0; j < P->Inputs.size(); j++) {
			  Node* S = P->Inputs[j];
			  if(S != N) {
			      if(OffMap.find(S) == OnMap.end())
				  OffMap[S] = S->OFF;
			      if(OnMap.find(P) == OnMap.end())
				  OnMap[P] = P->ON;
			      if(S != N) {
				  if(!UpdateOffMap(P, OffMap[N] * OffMap[S], OnMap, OffMap)) return false;
				  if(!UpdateOnMap(S, OffMap[N] * OnMap[P], OnMap, OffMap)) return false;
			      }
			  }
		      }
		  }
	      }
	  }
	  if(N->LogicOp == 'A') {
	      if(N->Inputs.size() == FaninBound) {
		  for(int i = 0; i < N->Inputs.size(); i++) {
		      BDD Cover = OffMap[N];
		      for(int j = 0; j < N->Inputs.size(); j++) {
			  if(N->Inputs[j] != N->Inputs[i]) {
			      if(OnMap.find(N->Inputs[j]) == OnMap.end())
				  OnMap[N->Inputs[j]] = N->Inputs[j]->ON;
			      Cover *= OnMap[N->Inputs[j]];
			  }
		      }
		      if(Cover != ZERO)
			  if(!UpdateOffMap(N->Inputs[i], Cover, OnMap, OffMap)) return false;
		  }
	      }
	  } else if(N->LogicOp == 'N') {
	      if(!N->Inputs.empty()) if(!UpdateOnMap(N->Inputs[0], OffMap[N], OnMap, OffMap)) return false;
	  } else if(N->LogicOp == 'A') {
	      for(int i = 0; i < N->Inputs.size(); i++)
		  if(!UpdateOffMap(N->Inputs[i], OffMap[N], OnMap, OffMap)) return false;
	  }
      } else if(GateType == 4) {
	  //And(P):  ON(P) <- ON(N) * ON(S)       OFF(P) <- OFF(N)
	  //        OFF(S) <- ON(N) * OFF(P) 
	  //Or(P):   ON(P) <- ON(N)               OFF(P) <- OFF(N) * OFF(S) 
	  //                                       ON(S) <- OFF(N) * ON(P)
	  //And(N): ON(I_1) <- ON(N)            OFF(I_1) <- OFF(N) * ON(I_2)
	  //Or(N):  ON(I_1) <- ON(N) * OFF(I_2) OFF(I_1) <- OFF(N)
	  
	  for(set<Node*>::iterator i = N->Parents.begin(); i != N->Parents.end(); i++) {
	      Node* P = *i;
	      if(P->LogicOp == 'A') {
		  if(!UpdateOffMap(P, OffMap[N], OnMap, OffMap)) return false;
	      } else if(P->LogicOp == 'O') {
		  if(P->Inputs.size() == FaninBound) {
		      for(int j = 0; j < P->Inputs.size(); j++) {
			  Node* S = P->Inputs[j];
			  if(S != N) {
			      if(OffMap.find(S) == OnMap.end())
				  OffMap[S] = S->OFF;
			      if(OnMap.find(P) == OnMap.end())
				  OnMap[P] = P->ON;
			      if(S != N) {
				  if(!UpdateOffMap(P, OffMap[N] * OffMap[S], OnMap, OffMap)) return false;
				  if(!UpdateOnMap(S, OffMap[N] * OnMap[P], OnMap, OffMap)) return false;
			      }
			  }
		      }
		  }
	      }
	  }
	  if(N->LogicOp == 'A') {
	      if(N->Inputs.size() == FaninBound) {
		  for(int i = 0; i < N->Inputs.size(); i++) {
		      BDD Cover = OffMap[N];
		      for(int j = 0; j < N->Inputs.size(); j++) {
			  if(N->Inputs[j] != N->Inputs[i]) {
			      if(OnMap.find(N->Inputs[j]) == OnMap.end())
				  OnMap[N->Inputs[j]] = N->Inputs[j]->ON;
			      Cover *= OnMap[N->Inputs[j]];
			  }
		      }
		      if(Cover != ZERO)
			  if(!UpdateOffMap(N->Inputs[i], Cover, OnMap, OffMap)) return false;
		  }
	      }
	  } else if(N->LogicOp == 'A') {
	      for(int i = 0; i < N->Inputs.size(); i++)
		  if(!UpdateOffMap(N->Inputs[i], OffMap[N], OnMap, OffMap)) return false;
	  }
      }
  }
  return true;
}


//////////////////////// MANIPULATORS /////////////////////////////////////////
Node* Network::FindNewNodeNeeded(BDD& Uncovered) {
    Uncovered = ZERO;
    for(int i = Internal.size()-1; i >= 0; i--) {
	Node* N = Internal[i];
	BDD Uncoverable = N->Uncovered; 
	if(Uncoverable != ZERO) 
	    if(GateType == 1)
		for(int j = 0; j < N->Connectible.size(); j++) Uncoverable *= N->Connectible[j]->ON;
	    else if(GateType == 2)
		for(int j = 0; j < N->Connectible.size(); j++) Uncoverable *= N->Connectible[j]->OFF;
	    else if((GateType == 3 || GateType == 4) && N->LogicOp == 'A')
		for(int j = 0; j < N->Connectible.size(); j++) Uncoverable *= N->Connectible[j]->ON;
	    else if((GateType == 3 ||GateType == 4) && N->LogicOp == 'O')
		for(int j = 0; j < N->Connectible.size(); j++) Uncoverable *= N->Connectible[j]->OFF;
	    else if(GateType == 3 && N->LogicOp == 'N')
		for(int j = 0; j < N->Connectible.size(); j++) Uncoverable *= N->Connectible[j]->OFF;
	    else if(GateType == 5 && N->LogicOp == 'D')
	      for(int j = 0; j < N->Connectible.size(); j++) Uncoverable *= N->Connectible[j]->ON;
	    else if(GateType == 5 && N->LogicOp == 'R')
	      for(int j = 0; j < N->Connectible.size(); j++) Uncoverable *= N->Connectible[j]->OFF;
	if(Uncoverable != ZERO) {
	    Uncovered = Uncoverable;
	    return N;
	}
    }
    for(int i = Outputs.size()-1; i >= 0; i--) {
	Node* N = Outputs[i];
	BDD Uncoverable = N->Uncovered; 
	if(Uncoverable != ZERO) 
	    if(GateType == 1) 
		for(int j = 0; j < N->Connectible.size(); j++) Uncoverable *= N->Connectible[j]->ON;
	    else if(GateType == 2)
		for(int j = 0; j < N->Connectible.size(); j++) Uncoverable *= N->Connectible[j]->OFF;
	    else if((GateType == 3 || GateType == 4) && N->LogicOp == 'A')
		for(int j = 0; j < N->Connectible.size(); j++) Uncoverable *= N->Connectible[j]->ON;
	    else if((GateType == 3 || GateType == 4) && N->LogicOp == 'O')
		for(int j = 0; j < N->Connectible.size(); j++) Uncoverable *= N->Connectible[j]->OFF;
	    else if(GateType == 3 && N->LogicOp == 'N')
		for(int j = 0; j < N->Connectible.size(); j++) Uncoverable *= N->Connectible[j]->OFF;
	    else if(GateType == 5 && N->LogicOp == 'D')
	      for(int j = 0; j < N->Connectible.size(); j++) Uncoverable &= N->Connectible[j]->ON;
	    else if(GateType == 5 && N->LogicOp == 'R')
	      for(int j = 0; j < N->Connectible.size(); j++) Uncoverable *= N->Connectible[j]->OFF;
	if(Uncoverable != ZERO) {
	    Uncovered = Uncoverable;
	    return N;
	}
    }
    return NULL;
}
void Network::AddNewNodes() {
  BDD Uncovered;
  Node* NewNodeNeeded = FindNewNodeNeeded(Uncovered);
  while(NewNodeNeeded != NULL) {
      Node* newNode = AddNewNodeInput(NewNodeNeeded, Uncovered);
      NewNodeNeeded = FindNewNodeNeeded(Uncovered);
  }
}

Node* Network::AddNewNodeInput(Node* NewNodeNeeded, BDD Uncovered) {
    bool tempPrint = print;
    print = false;
    Node* NodeAdded;
    Node* P = NewNodeNeeded;
    BDD MustCover = Uncovered;
    //This is an implication                                                                  
    ImplicationCount++;
    StrucImplCount++;
    //Get the new node and add as an input of P                                               
    Node* C = newNode();  //Will never be GateType = 3 or 4 or 5
    C->idx = vars++;
    Internal.push_back(C);
    if(P->Inputs.size() < FaninBound) P->Inputs.push_back(C);
    C->Parents.insert(P);
    
    //Update the level of C                                                                   
    if(P->level+1 > C->level) SetLevel(C, P->level+1);
    
    cost = ComputeCost();

    //Update the on/off-sets based on this connection.                                        
    bool UpdateOK = false;
    if(GateType == 1) {
	//ON(C) += OFF(P)          
	if(UpdateOnSet(C, P->OFF, NULL, 0)) {
	    //OFF(P) += minterm from MustCover                                                    
	    if(UpdateOffSet(C, PickMinterm(MustCover), NULL, 0)) {
		//If P has two children, additional updates are possible                          
		if(P->Inputs.size() == FaninBound) {
		    //OFF(P) += {*ON(I) : For all I in INPUT(P)}    Two inputs: OFF(P) = ON(C) * ON(S)
		    BDD OnProd = ONE;
		    for(int i = 0; i < P->Inputs.size(); i++) OnProd *= P->Inputs[i]->ON;
		    if(UpdateOffSet(P, OnProd, NULL, 0)) {
			//OFF(C) += ON(P) * {*ON(I) : For all I in INPUT(P), where I != C}    Two inputs: OFF(C) += ON(P) * ON(S)
			OnProd = P->ON;
			for(int i = 0; i < P->Inputs.size(); i++) if(P->Inputs[i] != C) OnProd *= P->Inputs[i]->ON;
			if(UpdateOffSet(C, OnProd, NULL, 0)) {
			    //OFF(S) += ON(P) * {*ON(I) : For all I in INPUT(P), where I != S}  Two inputs: OFF(S) += ON(P) * ON(C)
			    UpdateOK = true;
			    for(int i = 0; i < P->Inputs.size() && UpdateOK; i++) {
				Node* S = P->Inputs[i];
				if(S != C) {
				    OnProd = P->ON;
				    for(int j = 0; j < P->Inputs.size(); j++)
					if(P->Inputs[j] != S) OnProd *= P->Inputs[j]->ON;
				    if(UpdateOffSet(S, OnProd, NULL, 0))
					UpdateOK = true;
				    else { 	
					if(print) cout<<"  Conflict updating off set of "<<S<<endl;
					if(trace) status<<"  Conflict updating off set of "<<S<<endl;
					UpdateOK = false;
				    }
				}
			    }
			} else {
			    if(print) cout<<"  Conflict updating off set of "<<C<<endl;
			    if(trace) status<<"  Conflict updating off set of "<<C<<endl;
			}
		    } else {
			if(print) cout<<"  Conflict updating off set of "<<P<<endl;
			if(trace) status<<"  Conflict updating off set of "<<P<<endl;
		    }
		} else UpdateOK = true;
	    } else {
		if(print) cout<<"  Conflict updating off set of "<<C<<endl;
		if(trace) status<<"  Conflict updating off set of "<<C<<endl;
	    }
	} else {
	    if(print) cout<<"  Conflict updating on set of "<<C<<endl;
	    if(trace) status<<"  Conflict updating on set of "<<C<<endl;
	}
	if(tempPrint) { cout<<"  Need new node "<<C<<" to cover "; PrintFunction(C->OFF); cout<<" in "<<P<<endl; }
    } else if(GateType == 2) {
	//OFF(C) += ON(P)          
	if(UpdateOffSet(C, P->ON, NULL, 0)) {
	    //ON(C) += minterm from MustCover                                                    
	    if(UpdateOnSet(C, PickMinterm(MustCover), NULL, 0)) {
		//If P has two children, additional updates are possible                          
		if(P->Inputs.size() == FaninBound) {
		    //ON(P) += {*OFF(I) : For all I in INPUT(P)}    Two inputs: ON(P) = OFF(C) * OFF(S)
		    BDD OffProd = ONE;
		    for(int i = 0; i < P->Inputs.size(); i++) OffProd *= P->Inputs[i]->OFF;
		    if(UpdateOnSet(P, OffProd, NULL, 0)) {
			//OFF(C) += OFF(P) * {*OFF(I) : For all I in INPUT(P), where I != C}    Two inputs: ON(C) += OFF(P) * OFF(S)
			OffProd = P->OFF;
			for(int i = 0; i < P->Inputs.size(); i++) if(P->Inputs[i] != C) OffProd *= P->Inputs[i]->OFF;
			if(UpdateOnSet(C, OffProd, NULL, 0)) {
			    //ON(S) += OFF(P) * {*OFF(I) : For all I in INPUT(P), where I != S}  Two inputs: ON(S) += OFF(P) * OFF(C)
			    UpdateOK = true;
			    for(int i = 0; i < P->Inputs.size() && UpdateOK; i++) {
				Node* S = P->Inputs[i];
				if(S != C) {
				    OffProd = P->OFF;
				    for(int j = 0; j < P->Inputs.size(); j++)
					if(P->Inputs[j] != S) OffProd *= P->Inputs[j]->OFF;
				    if(UpdateOnSet(S, OffProd, NULL, 0))
					UpdateOK = true;
				    else { 	
					if(print) cout<<"  Conflict updating on set of "<<S<<endl;
					if(trace) status<<"  Conflict updating on set of "<<S<<endl;
					UpdateOK = false;
				    }
				}
			    }
			} else {
			    if(print) cout<<"  Conflict updating on set of "<<C<<endl;
			    if(trace) status<<"  Conflict updating on set of "<<C<<endl;
			}
		    } else {
			if(print) cout<<"  Conflict updating on set of "<<P<<endl;
			if(trace) status<<"  Conflict updating on set of "<<P<<endl;
		    }
		} else UpdateOK = true;
	    } else {
		if(print) cout<<"  Conflict updating on set of "<<C<<endl;
		if(trace) status<<"  Conflict updating on set of "<<C<<endl;
	    }
	} else {
	    if(print) cout<<"  Conflict updating off set of "<<C<<endl;
	    if(trace) status<<"  Conflict updating off set of "<<C<<endl;
	}
	if(tempPrint) { cout<<"  Need new node "<<C<<" to cover "; PrintFunction(C->ON); cout<<" in "<<P<<endl; }
	/*} else if(GateType == 3) {
	if(P->LogicOp == 'N') {
	    if(UpdateOnSet(P, C->OFF, NULL, 0)) {
		if(UpdateOffSet(P, C->ON, NULL, 0)) {
		    if(UpdateOnSet(C, P->OFF, NULL, 0)) {
			if(UpdateOffSet(C, P->ON, NULL, 0)) {
			    UpdateOK = true;
			} else {
			    if(print) cout<<"  Conflict updating off set of "<<C<<endl;
			    if(trace) status<<"  Conflict updating off set of "<<C<<endl;
			}
		    } else {
			if(print) cout<<"  Conflict updating on set of "<<C<<endl;
			if(trace) status<<"  Conflict updating on set of "<<C<<endl;
		    }
		} else {
		    if(print) cout<<"  Conflict updating off set of "<<P<<endl;
		    if(trace) status<<"  Conflict updating off set of "<<P<<endl;
		}
	    } else {
		if(print) cout<<"  Conflict updating on set of "<<P<<endl;
		if(trace) status<<"  Conflict updating on set of "<<P<<endl;
	    }

		    } else UpdateOK = true;
		} else {
		    if(print) cout<<"  Conflict updating function of "<<C<<endl;
		    if(trace) status<<"  Conflict updating function of "<<C<<endl;
		}
	    } else {
		if(print) cout<<"  Conflict updating function of "<<C<<endl;
		if(trace) status<<"  Conflict updating function of "<<C<<endl;
	    }
	    }*/
    } else if(GateType == 4) {
	//And: On(C) <-  On(P)
	//Or: Off(C) <- Off(P)
	if((P->LogicOp == 'A' && UpdateOnSet(C, P->ON, NULL, 0)) ||
	   (P->LogicOp == 'O' && UpdateOffSet(C, P->OFF, NULL, 0))) {
	    //Make sure the minterm from MustCover is covered
	    //And: Off(C) <- Selected Minterm
	    //Or:   On(C) <- Selected Minterm
	    if((P->LogicOp == 'A' && UpdateOffSet(C, PickMinterm(MustCover), NULL, 0)) ||
	       (P->LogicOp == 'O' && UpdateOnSet(C, PickMinterm(MustCover), NULL, 0))) {
		//If P has max allowed children, additional updates are possible
		if(P->Inputs.size() == FaninBound) {
		    //And: On(P) <-  On(I_1) & ... &  On(I_n)
		    //Or: Off(P) <- Off(I_1) & ... & Off(I_n)
		    BDD Product = ONE;
		    for(int i = 0; i < P->Inputs.size(); i++)
			if(P->LogicOp == 'A') Product *= P->Inputs[i]->ON;
			else if(P->LogicOp == 'O') Product *= P->Inputs[i]->OFF;
		    if((P->LogicOp == 'A' && UpdateOnSet(P, Product, NULL, 0)) ||
		       (P->LogicOp == 'O' && UpdateOffSet(P, Product, NULL, 0))){
			//And: Off(C) <- Off(P) &  On(I_1) & ... &  On(I_n)
			//Or:   On(C) <-  On(P) & Off(I_1) & ... & Off(I_n)
			if(P->LogicOp == 'A') Product = P->OFF;
			else if(P->LogicOp == 'O') Product = P->ON;
			for(int i = 0; i < P->Inputs.size(); i++) 
			    if(P->Inputs[i] != C) 
				if(P->LogicOp == 'A') Product *= P->Inputs[i]->ON;
				else if(P->LogicOp == 'O') Product *= P->Inputs[i]->OFF;
			if((P->LogicOp == 'A' && UpdateOffSet(C, Product, NULL, 0)) ||
			   (P->LogicOp == 'O' && UpdateOnSet(C, Product, NULL, 0))) {
			    UpdateOK = true;
			    //And: Off(S) <- Off(P) &  On(I_1) & ... &  On(I_n)
			    //Or:   On(S) <-  On(P) & Off(I_1) & ... & Off(I_n)
			    UpdateOK = true;
			    for(int i = 0; i < P->Inputs.size() && UpdateOK; i++) {
				Node* S = P->Inputs[i];
				if(S != C) {
				    if(P->LogicOp == 'A') Product = P->OFF;
				    else if(P->LogicOp == 'O') Product = P->ON;
				    for(int j = 0; j < P->Inputs.size(); j++)
					if(P->Inputs[j] != S) {
					    if(P->LogicOp == 'A') Product *= P->Inputs[j]->ON;
					    else if(P->LogicOp == 'O') Product *= P->Inputs[j]->OFF;
					    if((P->LogicOp == 'A' && UpdateOffSet(S, Product, NULL, 0)) ||
					       (P->LogicOp == 'O' && UpdateOnSet(S, Product, NULL, 0)))
						UpdateOK = true;
					    else { 	
						if(print) cout<<"  Conflict updating off set of "<<S<<endl;
						if(trace) status<<"  Conflict updating off set of "<<S<<endl;
						UpdateOK = false;
					    }
					}
				}
			    }
			} else {
			    if(print) cout<<"  Conflict updating function of "<<C<<endl;
			    if(trace) status<<"  Conflict updating function of "<<C<<endl;
			}
		    } else {
			if(print) cout<<"  Conflict updating function of "<<P<<endl;
			if(trace) status<<"  Conflict updating function of "<<P<<endl;
		    }
		} else UpdateOK = true;
	    } else {
		if(print) cout<<"  Conflict updating function of "<<C<<endl;
		if(trace) status<<"  Conflict updating function of "<<C<<endl;
	    }
	} else {
	    if(print) cout<<"  Conflict updating function of "<<C<<endl;
	    if(trace) status<<"  Conflict updating function of "<<C<<endl;
	}
    } else if(GateType == 5) {
	//Nand: On(C) <-  Off(P)
	//Nor: Off(C) <- Off(P)
	if((P->LogicOp == 'D' && UpdateOnSet(C, P->OFF, NULL, 0)) ||
	   (P->LogicOp == 'R' && UpdateOffSet(C, P->ON, NULL, 0))) {
	    //Make sure the minterm from MustCover is covered
	    //Nand: Off(C) <- Selected Minterm
	    //Or:   On(C) <- Selected Minterm
	    if((P->LogicOp == 'D' && UpdateOffSet(C, PickMinterm(MustCover), NULL, 0)) ||
	       (P->LogicOp == 'R' && UpdateOnSet(C, PickMinterm(MustCover), NULL, 0))) {
		//If P has max allowed children, additional updates are possible
		if(P->Inputs.size() == FaninBound) {
		    //Nand: Off(P) <-  On(I_1) & ... &  On(I_n)
		    //Nor: On(P) <- Off(I_1) & ... & Off(I_n)
		    BDD Product = ONE;
		    for(int i = 0; i < P->Inputs.size(); i++)
			if(P->LogicOp == 'D') Product *= P->Inputs[i]->ON;
			else if(P->LogicOp == 'R') Product *= P->Inputs[i]->OFF;
		    if((P->LogicOp == 'D' && UpdateOffSet(P, Product, NULL, 0)) ||
		       (P->LogicOp == 'R' && UpdateOnSet(P, Product, NULL, 0))){
			//Nand: Off(C) <- On(P) &  On(I_1) & ... &  On(I_n)
			//Nor:  Off(C) <- Off(P) & Off(I_1) & ... & Off(I_n)
			if(P->LogicOp == 'D') Product = P->ON;
			else if(P->LogicOp == 'R') Product = P->OFF;
			for(int i = 0; i < P->Inputs.size(); i++) 
			    if(P->Inputs[i] != C) 
				if(P->LogicOp == 'D') Product *= P->Inputs[i]->ON;
				else if(P->LogicOp == 'R') Product *= P->Inputs[i]->OFF;
			if((P->LogicOp == 'D' && UpdateOffSet(C, Product, NULL, 0)) ||
			   (P->LogicOp == 'R' && UpdateOnSet(C, Product, NULL, 0))) {
			    UpdateOK = true;
			    //Nand: Off(S) <- On(P) &  On(I_1) & ... &  On(I_n)
			    //Nor:   On(S) <- Off(P) & Off(I_1) & ... & Off(I_n)
			    UpdateOK = true;
			    for(int i = 0; i < P->Inputs.size() && UpdateOK; i++) {
				Node* S = P->Inputs[i];
				if(S != C) {
				    if(P->LogicOp == 'D') Product = P->ON;
				    else if(P->LogicOp == 'R') Product = P->OFF;
				    for(int j = 0; j < P->Inputs.size(); j++)
					if(P->Inputs[j] != S) {
					    if(P->LogicOp == 'D') Product *= P->Inputs[j]->ON;
					    else if(P->LogicOp == 'R') Product *= P->Inputs[j]->OFF;
					    if((P->LogicOp == 'D' && UpdateOffSet(S, Product, NULL, 0)) ||
					       (P->LogicOp == 'R' && UpdateOnSet(S, Product, NULL, 0)))
						UpdateOK = true;
					    else { 	
						if(print) cout<<"  Conflict updating off set of "<<S<<endl;
						if(trace) status<<"  Conflict updating off set of "<<S<<endl;
						UpdateOK = false;
					    }
					}
				}
			    }
			} else {
			    if(print) cout<<"  Conflict updating function of "<<C<<endl;
			    if(trace) status<<"  Conflict updating function of "<<C<<endl;
			}
		    } else {
			if(print) cout<<"  Conflict updating function of "<<P<<endl;
			if(trace) status<<"  Conflict updating function of "<<P<<endl;
		    }
		} else UpdateOK = true;
	    } else {
		if(print) cout<<"  Conflict updating function of "<<C<<endl;
		if(trace) status<<"  Conflict updating function of "<<C<<endl;
	    }
	} else {
	    if(print) cout<<"  Conflict updating function of "<<C<<endl;
	    if(trace) status<<"  Conflict updating function of "<<C<<endl;
	}
    }

    NodeAdded = C;
    UpdateConnectibleSets();  
    print = tempPrint;
    return NodeAdded;
}

//Compute Symmetry pairs on the inputs of the Network for the output functions
//Must have added all input nodes and output nodes before running
/* !!! */
void Network::ComputeSymmetries() {
  if(print) cout<<"Symmetries: ";
  for(int i = 0; i < Inputs.size(); i++) {
    for(int j = i+1; j < Inputs.size(); j++) {
	if(!ComplementedInputs || i % 2 == j % 2) {
	    //if(i % 2 == j % 2) {
	    bool keepPair = true;
	    for(int k = 0; k < Outputs.size() && keepPair; k++) {
		BDD T = Outputs[k]->ON.Constrain(Inputs[i]->ON);
		BDD F = Outputs[k]->ON.Constrain(!Inputs[i]->ON);
		BDD TF = T.Constrain(!Inputs[j]->ON);
		BDD FT = F.Constrain(Inputs[j]->ON);
		if(TF != FT) keepPair = false;
	    }
	    if(keepPair) {
		SymmetryPairs[Inputs[i]].insert(Inputs[j]);
		SymmetryPairs[Inputs[j]].insert(Inputs[i]);
		if(print) cout<<" ("<<Inputs[i]<<" ~ "<<Inputs[j]<<") ";
	    }
	}
    }
  }
  if(print) cout<<endl;
}

/* !!! */
//Inserts a new input node into the Network with function f
void Network::InsertInput(BDD f) {
  Node* N = new Node;
  NODECOUNT++;
  N->idx = vars++;
  N->level = -1;
  N->ON = f;
  N->OFF = !f;
  N->Uncovered = ZERO;
  Inputs.push_back(N);
  if(ComplementedInputs) {
      Node* N = new Node;
      NODECOUNT++;
      N->idx = vars++;
      N->level = -1;
      N->ON = !f;
      N->OFF = f;
      N->Uncovered = ZERO;
      Inputs.push_back(N);
  }
}

/* !!! */
//Inserts a new output node into the Network with function f
void Network::InsertOutput(BDD f, char LogicOp) {
  Node* N = new Node;
  NODECOUNT++;
  N->idx = vars++;
  N->level = 0;
  N->ON = f;
  N->OFF = !f;
  if(GateType == 1) N->Uncovered = f;
  else if(GateType == 2) N->Uncovered = !f;
  else if(GateType == 3) {
      N->LogicOp = LogicOp;
      if(LogicOp == 'A') N->Uncovered = !f;
      else if(LogicOp == 'O') N->Uncovered = f;
      else if(LogicOp == 'N') N->Uncovered = f;
  } else if(GateType == 4) {
      N->LogicOp = LogicOp;
      if(LogicOp == 'A') N->Uncovered = !f;
      else if(LogicOp == 'O') N->Uncovered = f;
  } else if(GateType == 5) {
    N->LogicOp = LogicOp;
    if(LogicOp == 'D') N->Uncovered = f;
    else if(LogicOp == 'R') N->Uncovered = !f;
  }
  Outputs.push_back(N);
}

/*main*/
/* !!! */
void Network::Initialization() {
    cost = ComputeCost();
    UpdateConnectibleSets();
    /** Do implications by adding new gates now **/
    if(StrucImpl && GateType != 3 && GateType != 4 && GateType != 5) AddNewNodes();
}

Node* newNode(char LogicOp) {
    Node* N = new Node;
    NODECOUNT++;
    N->idx = -1;
    N->level = -1;
    N->ON = ZERO;
    N->OFF = ZERO;
    N->Uncovered = ZERO;
    N->LogicOp = LogicOp;
    return N;
}

/////////////////////// OUTPUT ////////////////////////////////////////////////
/* !!! */
void Network::PrintPicRec(int level, Node* N, vector<string>& Pic, set<int>& Repeats) {
  bool Repeat = (Repeats.find(N->idx) != Repeats.end());
  Repeats.insert(N->idx);
  string Name;
  char sp = ' ';
  string spsingle = " ";
  string sps = "  ";
  if(N->idx == -2) Name = "0";
  else if(N->idx == -1) Name = "1";
  else Name = N->Name();
  if(Pic.size() < (level+1)*2) {
    for(int i = Pic.size(); i < (level+1)*2; i++) {
      Pic.push_back("");
      Pic[i].append(Pic[i-1].size(), sp);
    }
  }
  string Line1 = Pic[level*2];
  string Line2 = Pic[level*2+1];
  int inputSize = N->Inputs.size();
  if(!Repeat && inputSize > 1) {
      for(int i = 0; i < inputSize; i++)
	  PrintPicRec(level+1, N->Inputs[i], Pic, Repeats);
      string NextLine1 = Pic[(level+1)*2];
      string NextLine2 = Pic[(level+1)*2+1];
      
      int EndLen = NextLine1.find_last_not_of(spsingle, NextLine1.size());
      int BeginLen = NextLine1.find_last_of(spsingle, EndLen); 
      for(int i = 1; i < inputSize; i++) {
	  BeginLen = NextLine1.find_last_not_of(spsingle, BeginLen);
	  BeginLen = NextLine1.find_last_of(spsingle, BeginLen);
      }
      int DiffLen = BeginLen - Pic[level*2].size() + 1;
      if(DiffLen > 0){
	  Line1.append(DiffLen, sp);
	  Line2.append(DiffLen, sp);
      }
      DiffLen = EndLen - BeginLen - Name.size();
      if(DiffLen > 0) {
	  Line1.append(DiffLen/2,sp);
      }
      Line1 += Name;
      DiffLen = EndLen - BeginLen - (inputSize*2 - 1);
      if(DiffLen > 0) {
	  Line2.append(DiffLen/2, sp);
      } 
      int len = 0;
      for(int i = 0; i < inputSize; i++) {
	  if(i != 0) { Line2 += spsingle; len++; }
	  if(inputSize % 2 == 1 && i == (inputSize-1)/2) Line2 += "|";
	  else if(i < inputSize/2) Line2 += "/";
	  else Line2 += "\\";
	  len++;
      }
      if(Line2.size() < Line1.size()) {
	  DiffLen = Line1.size() - Line2.size();
	  Line2.append(DiffLen, sp); 
      } else if(Line1.size() < Line2.size()) {
	  DiffLen = Line2.size() - Line1.size();
	  Line1.append(DiffLen, sp);
      }
      DiffLen = NextLine1.size() - Line1.size();
      if(DiffLen > 0) {
	  Line1.append(DiffLen,sp);
	  Line2.append(DiffLen,sp);
      }
      Pic[level*2] = Line1;
      Pic[level*2+1] = Line2;
  } else if(!Repeat && inputSize == 1) {
      PrintPicRec(level+1, N->Inputs[0], Pic, Repeats);
      string NextLine1 = Pic[(level+1)*2];
      string NextLine2 = Pic[(level+1)*2+1];
      int EndLen = NextLine1.find_last_not_of(spsingle, NextLine1.size());
      int BeginLen = NextLine1.find_last_of(spsingle, EndLen);
      int DiffLen = BeginLen - Pic[level*2].size()+1;
      if(DiffLen > 0){
	  Line1.append(DiffLen, sp);
	  Line2.append(DiffLen, sp);
    }
    DiffLen = EndLen - BeginLen - Name.size();
    if(DiffLen > 0) {
      Line1.append(DiffLen,sp);
      Line2.append(DiffLen,sp);
    }
    Line1 += Name;
    Line2 += spsingle + "|" + spsingle;
    DiffLen = Name.size() - 3;
    if(DiffLen > 0) Line2.append(DiffLen, sp); 
    DiffLen = NextLine1.size() - Line1.size();
    if(DiffLen > 0) {
      Line1.append(DiffLen,sp);
      Line2.append(DiffLen,sp);
    }
    Pic[level*2] = Line1;
    Pic[level*2+1] = Line2;
  } else {
    Line1 += Name + sps;
    Line2.append(Name.size()+2, sp);
    for(int i = (level+1)*2; i < Pic.size(); i++)
      Pic[i].append(Name.size()+2,sp);
    Pic[level*2] = Line1;
    Pic[level*2+1] = Line2;
  }
}
/* !!! */
void Network::PrintOutput(ostream& out) {
    for(int i = 0; i < Outputs.size(); i++) {
	out<<"FUNCTION: "<<Number(Outputs[i]->ON)<<" = "; PrintFunction(Outputs[i]->ON, out); out<<endl;
    }
}

void Network::PrintCatalog(ostream& out) {
    for(int i = 0; i < Outputs.size(); i++) {
	Node* N = Outputs[i];
	if(i != 0) out<<"***";
	out<<N<<" = ";
	if(GateType == 1) out<<"NAND(";
	else if(GateType == 2) out<<"NOR(";
	else if(GateType == 3) {
	    if(N->LogicOp == 'A') out<<"AND(";
	    else if(N->LogicOp == 'O') out<<"OR(";
	    else if(N->LogicOp == 'N') out<<"NOT(";
	} else if(GateType == 4) {
	    if(N->LogicOp == 'A') out<<"AND(";
	    else if(N->LogicOp == 'O') out<<"OR(";
	} else if(GateType == 5) {
	  if(N->LogicOp == 'D') out<<"NAND(";
	  else if(N->LogicOp == 'R') out<<"NOR(";
	}

	for(int j = 0; j < N->Inputs.size(); j++) {
	    if(j != 0) out<<", ";
	    out<<N->Inputs[j];
	}
	out<<")";
    }
    for(int i = 0; i < Internal.size(); i++) {
	Node* N = Internal[i];
	out<<"***";
	out<<N<<" = ";
	if(GateType == 1) out<<"NAND(";
	else if(GateType == 2) out<<"NOR(";
	else if(GateType == 3) {
	    if(N->LogicOp == 'A') out<<"AND(";
	    else if(N->LogicOp == 'O') out<<"OR(";
	    else if(N->LogicOp == 'N') out<<"NOT(";
	} else if(GateType == 4) {
	    if(N->LogicOp == 'A') out<<"AND(";
	    else if(N->LogicOp == 'O') out<<"OR(";
	} else if(GateType == 5) {
	  if(N->LogicOp == 'D') out<<"NAND(";
	  else if(N->LogicOp == 'R') out<<"NOR(";
	}				  
	for(int j = 0; j < N->Inputs.size(); j++) {
	    if(j != 0) out<<", ";
	    out<<N->Inputs[j];
	}
	out<<")";
    }
}

void Network::PrintText(ostream& out) {
  out<<"-----------"<<idx<<"-----------\n";
  for(int i = 0; i < Outputs.size(); i++) {
    Node* N = Outputs[i];
    out<<"(";
    for(set<Node*>::iterator i = N->Parents.begin(); i != N->Parents.end(); i++) {
	if(i != N->Parents.begin()) out<<", ";
	out<<(*i);
    }
    out<<") = ";
    out<<N<<" = (";
    for(int i = 0; i < N->Inputs.size(); i++) {
	if(i != 0) out<<", ";
	out<<N->Inputs[i];
    }
    out<<") = "; PrintInterval(N->ON, N->OFF, out); out<<endl;
  }
  for(int i = 0; i < Internal.size(); i++) {
    Node* N = Internal[i];
    out<<"(";
    for(set<Node*>::iterator i = N->Parents.begin(); i != N->Parents.end(); i++) {
	if(i != N->Parents.begin()) out<<", ";
	out<<(*i);
    }
    out<<") = ";
    out<<N<<" = (";
    for(int i = 0; i < N->Inputs.size(); i++) {
	if(i != 0) out<<", ";
	out<<N->Inputs[i];
    }
    out<<") = "; PrintInterval(N->ON, N->OFF, out); out<<endl;
  }
  for(int i = 0; i < Inputs.size(); i++) {
    Node* N = Inputs[i];
    out<<"(";
    for(set<Node*>::iterator i = N->Parents.begin(); i != N->Parents.end(); i++) {
	if(i != N->Parents.begin()) out<<", ";
	out<<(*i);
    }
    out<<") = ";
    out<<N<<" = "; PrintInterval(N->ON, N->OFF, out); out<<endl;
  }
}
/* !!! */
void Network::PrintPicture(ostream& out) {
  if(Internal.size() < 30) {
      out<<"-----------"<<idx<<"-----------\n";
      vector<string> Pic;
      Pic.push_back("");
      for(int i = 0; i < Outputs.size(); i++) {
	  set<int> temp;
	  PrintPicRec(0, Outputs[i], Pic, temp);
      }
      for(int i = 0; i < Pic.size(); i++)
	  out<<Pic[i]<<endl;
      out<<"----------------------\n";
      out<<"Cost: "<<cost<<"("<<LowerBound()<<")  Level: "<<Level()<<endl;
      for(int i = 0; i < Outputs.size(); i++)
	  PrintNode(Outputs[i], out);
      for(int i = 0; i < Internal.size(); i++)
	  PrintNode(Internal[i], out);
      for(int i = 0; i < Inputs.size(); i++)
	  PrintNode(Inputs[i], out);
      out<<"-----------"<<idx<<"-----------\n";
  } else {
      out<<"-----------"<<idx<<"-----------\n";
      out<<"Cost: "<<cost<<"("<<LowerBound()<<")  Level: "<<Level()<<endl;
      for(int i = 0; i < Outputs.size(); i++)
	  PrintNode(Outputs[i], out);
      int ct = 0;
      for(int i = 0; i < Internal.size(); i++)
	  if(Internal[i]->Uncovered != ZERO) ct++;
      out<<"\tUncovered = "<<ct<<endl;
      for(int i = 0; i < Inputs.size(); i++)
	  PrintNode(Inputs[i], out);
      out<<"-----------"<<idx<<"-----------\n";

  }
}
/* !!! */
void PrintSimpleNode(Node* N, ostream& out) {
    if(N->Inputs.empty()) {
	out<<N;
	return;
    }
    if(GateType == 1)
	out<<"Na(";
    else if(GateType == 2)
	out<<"No(";
    else if(GateType == 3 || GateType == 4 || GateType == 5)
	out<<N->LogicOp<<"(";
    for(int i = 0; i < N->Inputs.size(); i++) {
	if(i != 0) out<<", ";
	PrintSimpleNode(N->Inputs[i], out);
    }
    out<<")";
}
/* !!! */
void Network::PrintSimpleSolution(bool complete, ostream& out) {
  for(int i = 0; i < Outputs.size(); i++) {
    Node* N = Outputs[i];
    out<<Number(N->ON)<<";";
    PrintFunction(N->ON,out);
    if(complete) {
	out<<";";
	if(cost < 15)
	    PrintSimpleNode(Outputs[i],out);
	out<<";";
    } else {
	out<<";;";
    }
  }
  if(complete) {
      int level = Level();
      out<<cost<<";"<<level<<";";
  } else {
      out<<";;";
  }
}
////////////////////// ACCESSORS //////////////////////////////////////////////
bool Network::TreeStructure() {
    for(int i = 0; i < Outputs.size(); i++)
	if(Outputs[i]->Parents.size() > 1) return false;
    for(int i = 0; i < Internal.size(); i++)
	if(Internal[i]->Parents.size() > 1) return false;
    return true;
}

bool Network::IsSymmetric(BDD F, BDD G) {
    //cout<<"  IsSymmetric("; PrintFunction(F); cout<<", "; PrintFunction(G); cout<<")?";
    for(int i = 0; i < Inputs.size(); i++) {
	for(set<Node*>::iterator j = SymmetryPairs[Inputs[i]].begin(); j != SymmetryPairs[Inputs[i]].end(); j++) {
	    BDD F1 = F.Constrain(Inputs[i]->ON);
	    BDD F0 = F.Constrain(!Inputs[i]->ON);
	    BDD F10 = F1.Constrain(!(*j)->ON);
	    BDD F01 = F0.Constrain((*j)->ON);
	    BDD F00 = F0.Constrain(!(*j)->ON);
	    BDD F11 = F1.Constrain((*j)->ON);
	    BDD G1 = G.Constrain(Inputs[i]->ON);
	    BDD G0 = G.Constrain(!Inputs[i]->ON);
	    BDD G10 = G1.Constrain(!(*j)->ON);
	    BDD G01 = G0.Constrain((*j)->ON);
	    BDD G00 = G0.Constrain(!(*j)->ON);
	    BDD G11 = G1.Constrain((*j)->ON);
	    if(F00 == G00 && F10 == G01 && F01 == G10 && F11 == G11) {
		//cout<<"  yes"<<endl;
		return true;
	    }
	}
    }
    //cout<<"  no"<<endl;
    return false;
}
/* !!! */
//Remove an input from connectible set if neither symmetric input has been used in the network.
vector<Node*> Network::RemoveSymmetric(Node* N, vector<Node*>& Connections, map<Node*,vector<Node*> >& Removed) {
  vector<bool> Remove(Connections.size(), false);
  bool RemoveOne = false;
  for(int i = 0; i < Connections.size(); i++) {
    Node* C = Connections[i];
    if(IsInput(C) && C->Parents.empty()) {  //C must be an input and never used previous to this
      for(int j = i+1; j < Connections.size(); j++) {
	Node* D = Connections[j];
	if(IsInput(D) && D->Parents.empty()) { //D must be an input and never used previous to this
	  BDD WillCoverC, MayCoverC, WillCoverD, MayCoverD;
	  if(GateType == 1) {
	    WillCoverC = N->Uncovered * C->OFF; WillCoverD = N->Uncovered * D->OFF;
	    MayCoverC = N->Uncovered * !C->ON * !C->OFF; MayCoverD = N->Uncovered * !D->ON * !D->OFF;
	  } else if(GateType == 2) {
	    WillCoverC = N->Uncovered * C->ON; WillCoverD = N->Uncovered * D->ON;
	    MayCoverC = N->Uncovered * !C->OFF * !C->ON; MayCoverD = N->Uncovered * !D->OFF * !D->ON;
	  } else if(GateType == 3) {
	    if(N->LogicOp == 'A') {
	      WillCoverC = N->Uncovered * C->OFF; WillCoverD = N->Uncovered * D->OFF;
	      MayCoverC = N->Uncovered * !C->OFF * !C->ON; MayCoverD = N->Uncovered * !D->OFF * !D->ON;
	    } else if(N->LogicOp == 'O') {
	      WillCoverC = N->Uncovered * C->ON; WillCoverD = N->Uncovered * D->ON;
	      MayCoverC = N->Uncovered * !C->OFF * !C->ON; MayCoverD = N->Uncovered * !D->OFF * !D->ON;
	    } else if(N->LogicOp == 'N') {
	      WillCoverC = N->Uncovered * C->OFF; WillCoverD = N->Uncovered * D->OFF;
	      MayCoverC = N->Uncovered * !C->OFF * !C->ON; MayCoverD = N->Uncovered * !D->OFF * !D->ON;
	    }
	  } else if(GateType == 4) {
	    if(N->LogicOp == 'A') {
	      WillCoverC = N->Uncovered * C->OFF; WillCoverD = N->Uncovered * D->OFF;
	      MayCoverC = N->Uncovered * !C->OFF * !C->ON; MayCoverD = N->Uncovered * !D->OFF * !D->ON;
	    } else if(N->LogicOp == 'O') {
	      WillCoverC = N->Uncovered * C->ON; WillCoverD = N->Uncovered * D->ON;
	      MayCoverC = N->Uncovered * !C->OFF * !C->ON; MayCoverD = N->Uncovered * !D->OFF * !D->ON;
	    }
	  } else if(GateType == 5) {
	    if(N->LogicOp == 'D') {
	      WillCoverC = N->Uncovered * C->OFF; WillCoverD = N->Uncovered * D->OFF;
	      MayCoverC = N->Uncovered * !C->ON * !C->OFF; MayCoverD = N->Uncovered * !D->ON * !D->OFF;
	    } else if(N->LogicOp == 'R') {
	      WillCoverC = N->Uncovered * C->ON; WillCoverD = N->Uncovered * D->ON;
	      MayCoverC = N->Uncovered * !C->OFF * !C->ON; MayCoverD = N->Uncovered * !D->OFF * !D->ON;
	    }
	  }
	  if(SymmetryPairs[C].find(D) != SymmetryPairs[C].end() && IsSymmetric(WillCoverC, WillCoverD) && IsSymmetric(MayCoverC, MayCoverD)) {  //C and D are symmetric
	    if(print) cout<<"  Symmetry: Don't keep "<<D<<" because of "<<C<<endl;
	    if(trace) status<<"  Symmetry: Don't keep "<<D<<" because of "<<C<<endl;
	    Remove[j] = true;
	    RemoveOne = true;
	    Removed[C].push_back(D);
	  }
	}
      }
    }
  }
  if(RemoveOne) {
    vector<Node*> newConnections;
    for(int i = 0; i < Connections.size(); i++) {
      if(!Remove[i])
	newConnections.push_back(Connections[i]);
    }
    return newConnections;
  } else {
    return Connections;
  }
}

int SizeofSubTree(Node* N) {
    int size = 0;
    if(!N->IsInput()) size++;
    for(int i = 0; i < N->Inputs.size(); i++) {
	size += SizeofSubTree(N->Inputs[i]);
    }
    return size;
}

int SizeofUncoveredSubTree(Node* N) {
    int size = 0;
    if(N->Uncovered != ZERO) size++;
    for(int i = 0; i < N->Inputs.size(); i++) {
	size += SizeofUncoveredSubTree(N->Inputs[i]);
    }
    return size;
}

bool Network::UncoveredNode() {
  for(int i = 0; i < Outputs.size(); i++) {
    if(Outputs[i]->Uncovered != ZERO) {
	return true;
    }
  }
  for(int i = 0; i < Internal.size(); i++) {
    if(Internal[i]->Uncovered != ZERO) {
	return true;
    }
  }
  return false;
}

/* !!! */
int Network::Level() {
  int level = 0;
  for(int i = 0; i < Outputs.size(); i++)
      if(Outputs[i]->level > level)
	  level = Outputs[i]->level;
  for(int i = 0; i < Internal.size(); i++)
      if(Internal[i]->level > level)
	  level = Internal[i]->level;
  for(int i = 0; i < Inputs.size(); i++)
      if(Inputs[i]->level > level)
	  level = Inputs[i]->level;
  return level;
}
int Network::Gates() {
    return Outputs.size() + Internal.size();
}
int Network::Edges() {
  int edges = 0;
  for(int i = 0; i < Outputs.size(); i++)
      edges += Outputs[i]->Inputs.size();
  for(int i = 0; i < Internal.size(); i++)
      edges += Internal[i]->Inputs.size();
  return edges;
}
//////////////////////////////////////////////////////////////////////////////
// Node Functions
//////////////////////////////////////////////////////////////////////////////
/* !!! */
bool StructureCompatible(Node* N, Node* C) {
    if(N == C)
	return false;
    else if(!C->IsInput() && FanoutRestrict && 
	    C->Parents.find(N) == C->Parents.end() && 
	    C->Parents.size() >= FanoutRestriction)
	return false;
    else if(N->Inputs.size() == FaninBound) {
	if(N->AlreadyConnected.find(C) == N->AlreadyConnected.end())
	    if(FindInput(N, C) > -1) return true;
	    else return false;
	else 
	    return false;
    } else {
	if(N->AlreadyConnected.find(C) == N->AlreadyConnected.end())
	    if(!Ancestor(C,N)) 
		return true;
	    else 
		return false;
	else 
	    return false;
    }
}
/* !!! */
bool OffSetCompatible(Node* N, Node* C) {
    if(GateType == 1) return (N->OFF * C->OFF == ZERO);    
    else if(GateType == 2) {
	if(N->Inputs.size()+1 < FaninBound) return true;
	BDD result = C->OFF * N->OFF;
	for(int i = 0; i < N->Inputs.size(); i++) {
	    if(N->Inputs[i] == C) return true;
	    result *= N->Inputs[i]->OFF;
	}
	return (result == ZERO);
    } else if(GateType == 3) {
	if(N->LogicOp == 'A')
	    return(N->ON * C->OFF == ZERO);
	else if(N->LogicOp == 'O') {
	    if(N->Inputs.size()+1 < FaninBound) return true;
	    BDD result = C->OFF * N->ON;
	    for(int i = 0; i < N->Inputs.size(); i++) {
		if(N->Inputs[i] == C) return true;
		result *= N->Inputs[i]->OFF;
	    }
	    return (result == ZERO);
	} else if(N->LogicOp == 'N') {
	    return(N->OFF * C->OFF == ZERO);
	}
    } else if(GateType == 4) {
	if(N->LogicOp == 'A')
	    return(N->ON * C->OFF == ZERO);
	else if(N->LogicOp == 'O') {
	    if(N->Inputs.size()+1 < FaninBound) return true;
	    BDD result = C->OFF * N->ON;
	    for(int i = 0; i < N->Inputs.size(); i++) {
		if(N->Inputs[i] == C) return true;
		result *= N->Inputs[i]->OFF;
	    }
	    return (result == ZERO);
	}
    } else if(GateType == 5) {
      if(N->LogicOp == 'D')
	return (N->OFF * C->OFF == ZERO);    
      else if(N->LogicOp == 'R') {
	if(N->Inputs.size()+1 < FaninBound) return true;
	BDD result = C->OFF * N->OFF;
	for(int i = 0; i < N->Inputs.size(); i++) {
	  if(N->Inputs[i] == C) return true;
	  result *= N->Inputs[i]->OFF;
	}
	return (result == ZERO);
      }
    }
}
/* !!! */
bool OnSetCompatible(Node* N, Node* C) {
    if(GateType == 1) {
	if(N->Inputs.size()+1 < FaninBound) return true;
	BDD result = C->ON * N->ON;
	for(int i = 0; i < N->Inputs.size(); i++) {
	    if(N->Inputs[i] == C) return true;
	    result *= N->Inputs[i]->ON;
	}
	return (result == ZERO);
    } else if(GateType == 2) return (N->ON * C->ON == ZERO);
    else if(GateType == 3) {
	if(N->LogicOp == 'O')
	    return(N->OFF * C->ON == ZERO);
	else if(N->LogicOp == 'A') {
	    if(N->Inputs.size()+1 < FaninBound) return true;
	    BDD result = C->ON * N->OFF;
	    for(int i = 0; i < N->Inputs.size(); i++) {
		if(N->Inputs[i] == C) return true;
		result *= N->Inputs[i]->ON;
	    }
	    return (result == ZERO);
	} else if(N->LogicOp == 'N') {
	    return(N->ON * C->ON == ZERO);
	}
    } else if(GateType == 4) {
	if(N->LogicOp == 'O')
	    return(N->OFF * C->ON == ZERO);
	else if(N->LogicOp == 'A') {
	    if(N->Inputs.size()+1 < FaninBound) return true;
	    BDD result = C->ON * N->OFF;
	    for(int i = 0; i < N->Inputs.size(); i++) {
		if(N->Inputs[i] == C) return true;
		result *= N->Inputs[i]->ON;
	    }
	    return (result == ZERO);
	}
    } else if(GateType == 5) {
      if(N->LogicOp == 'D') {
	if(N->Inputs.size()+1 < FaninBound) return true;
	BDD result = C->ON * N->ON;
	for(int i = 0; i < N->Inputs.size(); i++) {
	  if(N->Inputs[i] == C) return true;
	  result *= N->Inputs[i]->ON;
	}
	return (result == ZERO);
      } else if(N->LogicOp = 'R')
	return (N->ON * C->ON == ZERO);
    }
}

bool OnSetCompatible(Node* N, BDD ON) {
    if(GateType == 1) {
	if(N->Inputs.size()+1 < FaninBound) return true;
	BDD result = ON * N->ON;
	for(int j = 0; j < N->Inputs.size(); j++) result *= N->Inputs[j]->ON;
	return(result == ZERO);
    } else if(GateType == 2) return (N->ON * ON == ZERO);
    else if(GateType == 3) {
	if(N->LogicOp == 'O')
	    return(N->OFF * ON == ZERO);
	else if(N->LogicOp == 'A') {
	    if(N->Inputs.size()+1 < FaninBound) return true;
	    BDD result = ON * N->OFF;
	    for(int i = 0; i < N->Inputs.size(); i++)
		result *= N->Inputs[i]->ON;
	    return (result == ZERO);
	} else if(N->LogicOp == 'N') {
	    return(N->ON * ON == ZERO);
	}
    } else if(GateType == 4) {
	if(N->LogicOp == 'O')
	    return(N->OFF * ON == ZERO);
	else if(N->LogicOp == 'A') {
	    if(N->Inputs.size()+1 < FaninBound) return true;
	    BDD result = ON * N->OFF;
	    for(int i = 0; i < N->Inputs.size(); i++)
		result *= N->Inputs[i]->ON;
	    return (result == ZERO);
	}
    } else if(GateType == 5) {
      if(N->LogicOp == 'D') {
	if(N->Inputs.size()+1 < FaninBound) return true;
	BDD result = ON * N->ON;
	for(int j = 0; j < N->Inputs.size(); j++) result *= N->Inputs[j]->ON;
	return(result == ZERO);
      } else if(N->LogicOp == 'R')
	return (N->ON * ON == ZERO);
    }
}

bool OffSetCompatible(Node* N, BDD OFF) {
  if(GateType == 1) return (N->OFF * OFF == ZERO);    
  else if(GateType == 2) {
    if(N->Inputs.size()+1 < FaninBound) return true;
    BDD result = OFF * N->OFF;
    for(int i = 0; i < N->Inputs.size(); i++)
      result *= N->Inputs[i]->OFF;
    return (result == ZERO);
  } else if(GateType == 3) {
    if(N->LogicOp == 'A')
      return(N->ON * OFF == ZERO);
    else if(N->LogicOp == 'O') {
      if(N->Inputs.size()+1 < FaninBound) return true;
      BDD result = OFF * N->ON;
      for(int i = 0; i < N->Inputs.size(); i++)
	result *= N->Inputs[i]->OFF;
      return (result == ZERO);
    } else if(N->LogicOp == 'N') {
      return(N->OFF * OFF == ZERO);
    }
  } else if(GateType == 4) {
    if(N->LogicOp == 'A')
      return(N->ON * OFF == ZERO);
    else if(N->LogicOp == 'O') {
      if(N->Inputs.size()+1 < FaninBound) return true;
      BDD result = OFF * N->ON;
      for(int i = 0; i < N->Inputs.size(); i++)
	result *= N->Inputs[i]->OFF;
      return (result == ZERO);
    }
  } else if(GateType == 5) {
    if(N->LogicOp == 'D') return (N->OFF * OFF == ZERO);    
    else if(N->LogicOp == 'R') {
      if(N->Inputs.size()+1 < FaninBound) return true;
      BDD result = OFF * N->OFF;
      for(int i = 0; i < N->Inputs.size(); i++)
	result *= N->Inputs[i]->OFF;
      return (result == ZERO);
    }
  }
}

Node* FindRoot(Node* N) {
    if(N->Parents.empty()) return N;
    else
	return FindRoot(*N->Parents.begin());
}

vector<Node*> FindStructureCompatible(Node* R, Node* N) {
    vector<Node*> Comp;
    if(!R->IsInput()) {
	if(StructureCompatible(N, R))
	    Comp.push_back(R);
	vector<Node*> tempComp;
	for(int i = 0; i < R->Inputs.size(); i++) {
	    tempComp = FindStructureCompatible(R->Inputs[i], N);
	    Comp.insert(Comp.begin(), tempComp.begin(), tempComp.end());
	}
    }
    return Comp;
}

int CountConn(Node* N) {
    int count = 0;
    if(N->Inputs.size() < FaninBound) {
	Node* Root = FindRoot(N);
	vector<Node*> ConnectibleNodes = FindStructureCompatible(Root, N);
	for(int i = 0; i < InputBdds.size(); i++) {
	    if(OffSetCompatible(N, !InputBdds[i]) && OnSetCompatible(N, InputBdds[i]))
		count++; 
	}
	for(int i = 0; i < ConnectibleNodes.size(); i++) {
	    if(OffSetCompatible(N, ConnectibleNodes[i]->OFF) && OnSetCompatible(N, ConnectibleNodes[i]->ON))
		count++;
	}
    }
    return count;
}

int GetGateLabel(Node* N) {
    if(GateType == 4) {
	if(N->Uncovered == ZERO) return 0;    //Covered
	BDD M = N->Uncovered;
	vector<BDD> MintermLabels(9, ZERO);
	for(int i = 0; i < N->Inputs.size(); i++) {
	    if(N->LogicOp == 'A') {
		MintermLabels[0] += M * N->Inputs[i]->OFF;
		if(N->Inputs[i]->LogicOp == 'O') MintermLabels[1] += M * !N->Inputs[i]->ON * !N->Inputs[i]->OFF;
		if(N->Inputs[i]->LogicOp == 'A') MintermLabels[5] += M * !N->Inputs[i]->ON * !N->Inputs[i]->OFF;
		M = M * N->Inputs[i]->ON;
	    } else if(N->LogicOp == 'O') {
		MintermLabels[0] += N->Inputs[i]->ON;
		if(N->Inputs[i]->LogicOp == 'A') MintermLabels[1] += M * !N->Inputs[i]->ON * !N->Inputs[i]->OFF;
		if(N->Inputs[i]->LogicOp == 'O') MintermLabels[5] += M * !N->Inputs[i]->ON * !N->Inputs[i]->OFF;
		M = M * N->Inputs[i]->OFF;
	    }
	}	
	for(int i = 0; i < N->Connectible.size() && M != ZERO; i++) {
	    Node* C = N->Connectible[i];
	    if(N->LogicOp == 'A') {
		if(C->IsInput()) MintermLabels[2] += M * C->OFF;
		else if(C->LogicOp == 'O') {
		    MintermLabels[3] += M * C->OFF;
		    MintermLabels[4] += M * !C->ON * !C->OFF;
		} else if(C->LogicOp == 'A') {
		    MintermLabels[6] += M * C->OFF;
		    MintermLabels[7] += M * !C->ON * !C->OFF;
		}
		M = M * C->ON;
	    }
	    if(N->LogicOp == 'O') {
		if(C->IsInput()) MintermLabels[2] += M * C->ON;
		else if(C->LogicOp == 'A') {
		    MintermLabels[3] += M * C->ON;
		    MintermLabels[4] += M * !C->ON * !C->OFF;
		} else if(C->LogicOp == 'O') {
		    MintermLabels[6] += M * C->ON;
		    MintermLabels[7] += M * !C->ON * !C->OFF;
		}
		M = M * C->OFF;
	    }
	}
	MintermLabels[8] = M;
	BDD Covered = ZERO;
	int maxLabel = 0;
	//cout<<"GetGateLabel("<<N<<")"<<endl;
	for(int i = 0; i < 9; i++) {
	    BDD keep = !Covered * MintermLabels[i];
	    MintermLabels[i] = keep;
	    Covered = keep;
	    if(keep != ZERO) maxLabel = i;
	    //cout<<"  "<<i<<": "; PrintFunction(keep); cout<<endl;
	}
	return maxLabel;
    }
}

//Labels for when GateType = 4: {AND, OR}
//0: Covered
//1: Node with opposite gate type as input
//2: Primary input
//3: Node with opposite gate type with M in on/off-set
//4: Node with opposite gate type with M in dc-set
//5: Node with same gate type as input
//6: Node with same gate type with M in on/off-set
//7: Node with same gate type with M in dc-set
//8: New node with opposite gate type
//9: New node with same gate type
int GetLabel(Node* N, BDD& M) {
    if(GateType == 4) {
	//cout<<"***Get Label("<<N<<", "; PrintFunction(M); cout<<")***"<<endl;
	if(N->Uncovered * M == ZERO) return 0;    //Covered
	int minLabel = INF;
	for(int i = 0; i < N->Inputs.size(); i++)
	    if((N->LogicOp == 'A' && N->Inputs[i]->ON * M == ZERO) || //M can be added to off-set of input
	       (N->LogicOp == 'O' && N->Inputs[i]->OFF * M == ZERO))  //M can be added to on-set of input
		if(N->Inputs[i]->LogicOp != N->LogicOp)
		    return 1;                      //Node input with different gate type
		else if(N->Inputs[i]->LogicOp == N->LogicOp)
		    minLabel = 5;                  //Node input with same gate type
	for(int i = 0; i < N->Connectible.size(); i++) {
	    Node* C = N->Connectible[i];
	    if((N->LogicOp == 'A' && C->ON * M == ZERO) ||   //M can be added to off-set of input
	       (N->LogicOp == 'O' && C->OFF * M == ZERO)) {  //M can be added to on-set of input
		if(C->IsInput()) {
		    return 2;                      //Primary input
		} else if((N->LogicOp == 'A' && C->OFF * M != ZERO) || //M is already in off-set of node
			(N->LogicOp == 'O' && C->ON * M != ZERO)) {    //M is already in on-set of node
		    if(C->LogicOp != N->LogicOp && minLabel > 3)  //Node with opposite gate type
			minLabel = 3;
		    if(C->LogicOp == N->LogicOp && minLabel > 6)  //Node with same gate type
			minLabel = 6;
		} else {  // M must be added to on/off-set of node
		    if(C->LogicOp != N->LogicOp && minLabel > 4)  //Node with opposite gate type
			minLabel = 3;
		    if(C->LogicOp == N->LogicOp && minLabel > 7)  //Node with same gate type
			minLabel = 6;
		}
	    }
	}	       
	return min(minLabel, 8);
    }
}

void UpdateLabel(Node* N) {
    if(N->Uncovered == ZERO) {
	N->Label = 0;
	if(GateType == 1) N->UncoveredSet[0] = N->ON;
	else if(GateType == 2) N->UncoveredSet[0] = N->OFF;
	else if(GateType == 3) {
	    if(N->LogicOp == 'A') N->UncoveredSet[0] = N->OFF;
	    else if(N->LogicOp == 'O') N->UncoveredSet[0] = N->ON;
	    else if(N->LogicOp == 'N') N->UncoveredSet[0] = N->ON;
	} else if(GateType == 4) {
	    if(N->LogicOp == 'A') N->UncoveredSet[0] = N->OFF;
	    else if(N->LogicOp == 'O') N->UncoveredSet[0] = N->ON;
	} else if(GateType == 5) {
	  if(N->LogicOp == 'D') N->UncoveredSet[0] = N->ON;
	  else if(N->LogicOp == 'R') N->UncoveredSet[0] = N->OFF;
	}
	for(int i = 1; i < 5; i++)
	  N->UncoveredSet[i] = ZERO;
    } else {
	for(int i = 0; i < 5; i++)
	    N->UncoveredSet[i] = ZERO;
	if(GateType == 1) N->UncoveredSet[0] = N->ON * !N->Uncovered;
	if(GateType == 2) N->UncoveredSet[0] = N->OFF * !N->Uncovered;
	else if(GateType == 3) {
	    if(N->LogicOp == 'A') N->UncoveredSet[0] = N->OFF * !N->Uncovered;
	    else if(N->LogicOp == 'O') N->UncoveredSet[0] = N->ON * !N->Uncovered;
	    else if(N->LogicOp == 'N') N->UncoveredSet[0] = N->ON * !N->Uncovered;
	} else if(GateType == 4) {
	    if(N->LogicOp == 'A') N->UncoveredSet[0] = N->OFF * !N->Uncovered;
	    else if(N->LogicOp == 'O') N->UncoveredSet[0] = N->ON * !N->Uncovered;
	} else if(GateType == 5) {
	  if(N->LogicOp == 'D') N->UncoveredSet[0] = N->ON * !N->Uncovered;
	  else if(N->LogicOp == 'R') N->UncoveredSet[0] = N->OFF * !N->Uncovered;
	}
	BDD Leftover = N->Uncovered;
	BDD Current = ZERO;
	//1. FI = Covered by Fanin
	for(int i = 0; i < N->Inputs.size(); i++)
	    if(GateType == 1) Current += Leftover * !N->Inputs[i]->ON;
	    else if(GateType == 2) Current += Leftover * !N->Inputs[i]->OFF;
	    else if(GateType == 3) {
		if(N->LogicOp == 'A') Current += Leftover * !N->Inputs[i]->ON;
		else if(N->LogicOp == 'O') Current += Leftover * !N->Inputs[i]->OFF;
		else if(N->LogicOp == 'N') Current += Leftover * !N->Inputs[i]->ON;
	    } else if(GateType == 4) {
		if(N->LogicOp == 'A') Current += Leftover * !N->Inputs[i]->ON;
		else if(N->LogicOp == 'O') Current += Leftover * !N->Inputs[i]->OFF;
	    } else if(GateType == 5) {
	      if(N->LogicOp == 'D') Current += Leftover * !N->Inputs[i]->ON;
	      else if(N->LogicOp == 'R') Current += Leftover * !N->Inputs[i]->OFF;
	    }
	N->UncoveredSet[1] = Current;
	Leftover *= !Current;
	if(Leftover != ZERO) {
	    BDD CurrentIN = ZERO;
	    BDD CurrentGN = ZERO;
	    //2. IN = Covered by Input
	    //3. GN = Covered by existing Gate node
	    for(int i = 0; i < N->Connectible.size(); i++) {
		if(N->Connectible[i]->idx > -1 && N->Connectible[i]->IsInput()) {
		    //Input Node
		    if(GateType == 1) CurrentIN += Leftover * !N->Connectible[i]->ON;
		    else if(GateType == 2) CurrentIN += Leftover * !N->Connectible[i]->OFF;
		    else if(GateType == 3) {
			if(N->LogicOp == 'A') CurrentIN += Leftover * !N->Connectible[i]->ON;
			else if(N->LogicOp == 'O') CurrentIN += Leftover * !N->Connectible[i]->OFF;
			else if(N->LogicOp == 'N') CurrentIN += Leftover * !N->Connectible[i]->ON;
		    } else if(GateType == 4) {
			if(N->LogicOp == 'A') CurrentIN += Leftover * !N->Connectible[i]->ON;
			else if(N->LogicOp == 'O') CurrentIN += Leftover * !N->Connectible[i]->OFF;
		    } else if(GateType == 5) {
		      if(N->LogicOp == 'D') CurrentIN += Leftover * !N->Connectible[i]->ON;
		      else if(N->LogicOp == 'R') CurrentIN += Leftover * !N->Connectible[i]->OFF;
		    }
		} else if(!N->IsFanin(N->Connectible[i]))
		    //existing Gate Node
		    if(GateType == 1) CurrentGN += Leftover * !N->Connectible[i]->ON;
		    else if(GateType == 2) CurrentGN += Leftover * !N->Connectible[i]->OFF;
		    else if(GateType == 3) {
			if(N->LogicOp == 'A') CurrentGN += Leftover * !N->Connectible[i]->ON;
			else if(N->LogicOp == 'O') CurrentGN += Leftover * !N->Connectible[i]->OFF;
			else if(N->LogicOp == 'N') CurrentGN += Leftover * !N->Connectible[i]->ON;
		    } else if(GateType == 4) {
			if(N->LogicOp == 'A') CurrentGN += Leftover * !N->Connectible[i]->ON;
			else if(N->LogicOp == 'O') CurrentGN += Leftover * !N->Connectible[i]->OFF;
		    } else if(GateType == 5) {
		      if(N->LogicOp == 'D') CurrentGN += Leftover * !N->Connectible[i]->ON;
		      else if(N->LogicOp == 'R') CurrentGN += Leftover * !N->Connectible[i]->OFF;
		    }
	    }
	    N->UncoveredSet[2] = CurrentIN;
	    Leftover *= !CurrentIN;
	    if(Leftover != ZERO) {
		CurrentGN *= Leftover;
		N->UncoveredSet[3] = CurrentGN;
		Leftover *= !CurrentIN * !CurrentGN;
		if(Leftover != ZERO) {
		    N->UncoveredSet[4] = Leftover;
		    N->Label = 4;
		} else
		    N->Label = 3;
	    } else
		N->Label = 2;
	} else
	    N->Label = 1;
    }
/*
    cout<<"UpdateLabel"<<endl;
    cout<<"  "<<N<<"("<<N->Label<<"): "; PrintFunction(N->Uncovered); cout<<endl;
    for(int i = 0; i < 5; i++) {
	cout<<"    ["<<i<<"]: "; PrintFunction(N->UncoveredSet[i]); cout<<endl;
    }
*/
}

void UpdateLabelKL(Node* N) {
    if(N->Uncovered == ZERO) {
	N->Label = 0;
	if(GateType == 1) N->UncoveredSet[0] = N->ON;
	else if(GateType == 2) N->UncoveredSet[0] = N->OFF;
	else if(GateType == 3) {
	    if(N->LogicOp == 'A') N->UncoveredSet[0] = N->OFF;
	    else if(N->LogicOp == 'O') N->UncoveredSet[0] = N->ON;
	    else if(N->LogicOp == 'N') N->UncoveredSet[0] = N->ON;
	} else if(GateType == 4) {
	    if(N->LogicOp == 'A') N->UncoveredSet[0] = N->OFF;
	    else if(N->LogicOp == 'O') N->UncoveredSet[0] = N->ON;
	}
	for(int i = 1; i < 5; i++)
	    N->UncoveredSet[i] = ZERO;
    } else {
	for(int i = 0; i < 5; i++)
	    N->UncoveredSet[i] = ZERO;
	if(GateType == 1) N->UncoveredSet[0] = N->ON * !N->Uncovered;
	else if(GateType == 2) N->UncoveredSet[0] = N->OFF * !N->Uncovered;
	else if(GateType == 3) {
	    if(N->LogicOp == 'A') N->UncoveredSet[0] = N->OFF * !N->Uncovered;
	    else if(N->LogicOp == 'O') N->UncoveredSet[0] = N->ON * !N->Uncovered;
	    else if(N->LogicOp == 'N') N->UncoveredSet[0] = N->ON * !N->Uncovered;
	} else if(GateType == 4) {
	    if(N->LogicOp == 'A') N->UncoveredSet[0] = N->OFF * !N->Uncovered;
	    else if(N->LogicOp == 'O') N->UncoveredSet[0] = N->ON * !N->Uncovered;
	}
	BDD Leftover = N->Uncovered;
	//1. IN = Covered by Input
	BDD CurrentIN = ZERO;
	BDD CurrentGN = ZERO;
	for(int i = 0; i < N->Connectible.size(); i++) {
	    if(N->Connectible[i]->idx > -1 && N->Connectible[i]->IsInput()) {
		if(GateType == 1) CurrentIN += Leftover * !N->Connectible[i]->ON;
		else if(GateType == 2) CurrentIN += Leftover * !N->Connectible[i]->OFF;
	    } else if(!N->IsFanin(N->Connectible[i]))
		if(GateType == 1) CurrentGN += Leftover * !N->Connectible[i]->ON;
		else if(GateType == 2) CurrentGN += Leftover * !N->Connectible[i]->OFF;
		else if(GateType == 3) {
		    if(N->LogicOp == 'A') CurrentGN += Leftover * !N->Connectible[i]->ON;
		    else if(N->LogicOp == 'O') CurrentGN += Leftover * !N->Connectible[i]->OFF;
		    else if(N->LogicOp == 'N') CurrentGN += Leftover * !N->Connectible[i]->ON;
		} else if(GateType == 4) {
		    if(N->LogicOp == 'A') CurrentGN += Leftover * !N->Connectible[i]->ON;
		    else if(N->LogicOp == 'O') CurrentGN += Leftover * !N->Connectible[i]->OFF;
		}
	}
	N->UncoveredSet[1] = CurrentIN;
	Leftover *= !CurrentIN;
	//2. FI = Covered by Fanin
	if(Leftover != ZERO) {
	    BDD Current = ZERO;
	    for(int i = 0; i < N->Inputs.size(); i++)
		if(GateType == 1) Current += Leftover * !N->Inputs[i]->ON;
		else if(GateType == 2) Current += Leftover * !N->Inputs[i]->OFF;
		else if(GateType == 3) {
		    if(N->LogicOp == 'A') Current += Leftover * !N->Inputs[i]->ON;
		    else if(N->LogicOp == 'O') Current += Leftover * !N->Inputs[i]->OFF;
		    else if(N->LogicOp == 'N') Current += Leftover * !N->Inputs[i]->ON;
		} else if(GateType == 4) {
		    if(N->LogicOp == 'A') Current += Leftover * !N->Inputs[i]->ON;
		    else if(N->LogicOp == 'O') Current += Leftover * !N->Inputs[i]->OFF;
		}
	    N->UncoveredSet[2] = Current;
	    Leftover *= !Current;
	    //3. GN = Covered by existing Gate node
	    if(Leftover != ZERO) {
		CurrentGN *= Leftover;
		N->UncoveredSet[3] = CurrentGN;
		Leftover *= !CurrentIN * !CurrentGN;
		if(Leftover != ZERO) {
		    N->UncoveredSet[4] = Leftover;
		    N->Label = 4;
		} else
		    N->Label = 3;
	    } else
		N->Label = 2;
	} else
	    N->Label = 1;
    }
}

void Network::UpdateConnectibleSets() {
    UpdateUncovered();
    for(int i = 0; i < Outputs.size(); i++) {
	if(Outputs[i]->Uncovered != ZERO) {
	    Outputs[i]->Connectible = FindConnectible(Outputs[i]);
	} else {
	    Outputs[i]->Connectible.clear();
	}
	if(MintermChoice > 'H' && MintermChoice < 'P')
	    if(Outputs[i]->Uncovered != ZERO || Outputs[i]->Label != 0) 
		if(MintermChoice == 'K' || MintermChoice == 'L' || MintermChoice == 'N')
		    UpdateLabelKL(Outputs[i]);
		else
		    UpdateLabel(Outputs[i]);
    }
    for(int i = 0; i < Internal.size(); i++) {
	if(Internal[i]->Uncovered != ZERO) {
	    Internal[i]->Connectible = FindConnectible(Internal[i]);
	} else {
	    Internal[i]->Connectible.clear();
	}
	if(MintermChoice > 'H' && MintermChoice < 'P')
	    if(Internal[i]->Uncovered != ZERO || Internal[i]->Label != 0)
		if(MintermChoice == 'K' || MintermChoice == 'L' || MintermChoice == 'N')
		    UpdateLabelKL(Internal[i]);
		else
		    UpdateLabel(Internal[i]);
    }
}

void Randomize(vector<Node*>& NodeSet) {
  int total = NodeSet.size();
  vector<Node*> RandomSet(total);
  for(int i = 0; i < NodeSet.size(); i++) {
    double r = (double)rand() / ((double)(RAND_MAX)+(double)(1));
    double x = r * total;
    int index = (int)x;
    for(int j = 0; j < NodeSet.size() && index > -1; j++) {
      if(RandomSet[j] == NULL) {
	if(index == 0) {
	  RandomSet[j] = NodeSet[i];
	} 
	index--;
      }
    }
    total--;
  }
  NodeSet = RandomSet;
}

/* !!! */
//NetworkOrder: A,C = connectible order: primary inputs, fan-in, gate nodes, new
//              B,D = fan-in, primary inputs, gate nodes, new
//              E,F = Randomply Order connectible set
vector<Node*> Network::FindConnectible(Node* N) {
    vector<Node*> CoverNodes;
    BDD ToCover = N->Uncovered;
    //if(print) cout<<"--------------------------------------------------------------------------------------------"<<endl;
    //if(trace) status<<"--------------------------------------------------------------------------------------------"<<endl;
    //if(print) cout<<N<<" NODE\tFUNCTION\tOFF\tON\tCOVER"<<endl;
    //if(trace) status<<N<<" NODE\tFUNCTION\tOFF\tON\tCOVER"<<endl;
    //if(print) cout<<"--------------------------------------------------------------------------------------------"<<endl;
    //if(trace) status<<"--------------------------------------------------------------------------------------------"<<endl;
    if(NetworkOrder < 'E') {
	if(NetworkOrder == 'A' || NetworkOrder == 'C') {
	    if(N->Inputs.size() < FaninBound) {
		for(int i = 0; i < Inputs.size(); i++) {
		    if(StructureCompatible(N, Inputs[i])) {
			//if(print) { cout<<"     "<<Inputs[i]<<"\t"; PrintInterval(Inputs[i]->ON, Inputs[i]->OFF); cout<<"\t"; }
			//if(trace) { status<<"     "<<Inputs[i]<<"\t"; PrintInterval(Inputs[i]->ON, Inputs[i]->OFF, status); status<<"\t"; }
			if(OffSetCompatible(N, Inputs[i])) {
			    //if(print) cout<<"yes\t";
			    //if(trace) status<<"yes\t";
			    if(OnSetCompatible(N, Inputs[i])) {
				//if(print) cout<<"yes\t";
				//if(trace) status<<"yes\t";
				BDD totalcover;
				if(GateType == 1) totalcover = ToCover * !Inputs[i]->ON;
				else if(GateType == 2) totalcover = ToCover * !Inputs[i]->OFF;
				else if(GateType == 3) {
				    if(N->LogicOp == 'A') totalcover = ToCover * !Inputs[i]->ON;
				    else if(N->LogicOp == 'O') totalcover = ToCover * !Inputs[i]->OFF;
				    else if(N->LogicOp == 'N') totalcover = ToCover * !Inputs[i]->ON;
				} else if(GateType == 4) {
				    if(N->LogicOp == 'A') totalcover = ToCover * !Inputs[i]->ON;
				    else if(N->LogicOp == 'O') totalcover = ToCover * !Inputs[i]->OFF;
				} else if(GateType == 5) {
				  if(N->LogicOp == 'D') totalcover = ToCover * !Inputs[i]->ON;
				  else if(N->LogicOp == 'R') totalcover = ToCover * !Inputs[i]->OFF;
				}				
				//if(print) { PrintFunction(totalcover); cout<<endl; }
				//if(trace) { PrintFunction(totalcover,status); status<<endl; }
				if(totalcover != ZERO)
				    CoverNodes.push_back(Inputs[i]);
			    } else {
				//if(print) cout<<"no"<<endl;
				//if(trace) status<<"no"<<endl;
			    }
			} else {
			    //if(print) cout<<"no"<<endl;
			    //if(trace) status<<"no"<<endl;
			}
		    }
		}
	    }
	}
	for(int i = 0; i < N->Inputs.size(); i++) {
	    Node* C = N->Inputs[i];
	    if(!C->IsInput()) {
		if(StructureCompatible(N, C)) {
		    //if(print) { cout<<"     "<<C<<"\t"; PrintInterval(C->ON, C->OFF); cout<<"\t"; } 
		    //if(trace) { status<<"     "<<C<<"\t"; PrintInterval(C->ON, C->OFF, status); status<<"\t"; } 
		    if(OffSetCompatible(N, C)) {
			//if(print) cout<<"yes\t";
			//if(trace) status<<"yes\t";
			if(OnSetCompatible(N, C)) {
			    //if(print) cout<<"yes\t";
			    //if(trace) status<<"yes\t";
			    BDD totalcover;
			    if(GateType == 1) totalcover = ToCover * !C->ON;
			    else if(GateType == 2) totalcover = ToCover * !C->OFF;
			    else if(GateType == 3) {
				if(N->LogicOp == 'A') totalcover = ToCover * !C->ON;
				else if(N->LogicOp == 'O') totalcover = ToCover * !C->OFF;
				else if(N->LogicOp == 'N') totalcover = ToCover * !C->ON;
			    } else if(GateType == 4) {
				if(N->LogicOp == 'A') totalcover = ToCover * !C->ON;
				else if(N->LogicOp == 'O') totalcover = ToCover * !C->OFF;
			    } else if(GateType == 5) {
			      if(N->LogicOp == 'D') totalcover = ToCover * !C->ON;
			      else if(N->LogicOp == 'R') totalcover = ToCover * !C->OFF;
			    }
			    //if(print) { PrintFunction(totalcover); cout<<endl; }
			    ///if(trace) { PrintFunction(totalcover,status); status<<endl; }
			    if(totalcover != ZERO)
				CoverNodes.push_back(C);
			} else {
			    //if(print) cout<<"no"<<endl;
			    //if(trace) status<<"no"<<endl;
			}
		    } else {
			//if(print) cout<<"no"<<endl;
			//if(trace) status<<"no"<<endl;
		    }
		}
	    }
	}
       	if(NetworkOrder == 'B' || NetworkOrder == 'D') {
	    if(N->Inputs.size() < FaninBound) {
		for(int i = 0; i < Inputs.size(); i++) {
		    if(StructureCompatible(N, Inputs[i])) {
			//if(print) { cout<<"     "<<Inputs[i]<<"\t"; PrintInterval(Inputs[i]->ON, Inputs[i]->OFF); cout<<"\t"; }
			//if(trace) { status<<"     "<<Inputs[i]<<"\t"; PrintInterval(Inputs[i]->ON, Inputs[i]->OFF, status); status<<"\t"; }
			if(OffSetCompatible(N, Inputs[i])) {
			    //if(print) cout<<"yes\t";
			    //if(trace) status<<"yes\t";
			    if(OnSetCompatible(N, Inputs[i])) {
				//if(print) cout<<"yes\t";
				//if(trace) status<<"yes\t";
				BDD totalcover;
				if(GateType == 1) totalcover = ToCover * !Inputs[i]->ON;
				else if(GateType == 2) totalcover = ToCover * !Inputs[i]->OFF;
				else if(GateType == 3) {
				    if(N->LogicOp == 'A') totalcover = ToCover * !Inputs[i]->ON;
				    else if(N->LogicOp == 'O') totalcover = ToCover * !Inputs[i]->OFF;
				    else if(N->LogicOp == 'N') totalcover = ToCover * !Inputs[i]->ON;
				} else if(GateType == 4) {
				    if(N->LogicOp == 'A') totalcover = ToCover * !Inputs[i]->ON;
				    else if(N->LogicOp == 'O') totalcover = ToCover * !Inputs[i]->OFF;
				} else if(GateType == 5) {
				  if(N->LogicOp == 'D') totalcover = ToCover * !Inputs[i]->ON;
				  else if(N->LogicOp == 'R') totalcover = ToCover * !Inputs[i]->OFF;
				}
				//if(print) { PrintFunction(totalcover); cout<<endl; }
				//if(trace) { PrintFunction(totalcover,status); status<<endl; }
				if(totalcover != ZERO)
				    CoverNodes.push_back(Inputs[i]);
			    } else {
				//if(print) cout<<"no"<<endl;
				//if(trace) status<<"no"<<endl;
			    }
			} else {
			    //if(print) cout<<"no"<<endl;
			    //if(trace) status<<"no"<<endl;
			}
		    }
		}
	    }
	}
	if(N->Inputs.size() < FaninBound) {
	    for(int i = 0; i < Internal.size(); i++) {
		bool IsInput = false;
		for(int j = 0; j < N->Inputs.size() && !IsInput; j++)
		    if(N->Inputs[j] == Internal[i]) IsInput = true;
		if(!IsInput) {
		    if(StructureCompatible(N, Internal[i])) {
			//if(print) { cout<<"     "<<Internal[i]<<"\t"; PrintInterval(Internal[i]->ON, Internal[i]->OFF); cout<<"\t"; }
			//if(trace) { status<<"     "<<Internal[i]<<"\t"; PrintInterval(Internal[i]->ON, Internal[i]->OFF, status); status<<"\t"; }
			if(OffSetCompatible(N, Internal[i])) {
			    //if(print) cout<<"yes\t";
			    //if(trace) status<<"yes\t";
			    if(OnSetCompatible(N, Internal[i])) {
				//if(print) cout<<"yes\t";
				//if(trace) status<<"yes\t";
				BDD totalcover;
				if(GateType == 1) totalcover = ToCover * !Internal[i]->ON;
				else if(GateType == 2) totalcover = ToCover * !Internal[i]->OFF;
				else if(GateType == 3) {
				    if(N->LogicOp == 'A') totalcover = ToCover * !Internal[i]->ON;
				    else if(N->LogicOp == 'O') totalcover = ToCover * !Internal[i]->OFF;
				    else if(N->LogicOp == 'N') totalcover = ToCover * !Internal[i]->ON;
				} else if(GateType == 4) {
				    if(N->LogicOp == 'A') totalcover = ToCover * !Internal[i]->ON;
				    else if(N->LogicOp == 'O') totalcover = ToCover * !Internal[i]->OFF;
				} else if(GateType == 5) {
				  if(N->LogicOp == 'D') totalcover = ToCover * !Internal[i]->ON;
				  else if(N->LogicOp == 'R') totalcover = ToCover * !Internal[i]->OFF;
				}
				//if(print) { PrintFunction(totalcover); cout<<endl; }
				//if(trace) { PrintFunction(totalcover,status); status<<endl; }
				if(totalcover != ZERO)
				    CoverNodes.push_back(Internal[i]);
			    } else {
				//if(print) cout<<"no"<<endl;
				//if(trace) status<<"no"<<endl;
			    }
			} else {
			    //if(print) cout<<"no"<<endl;
			    //if(trace) status<<"no"<<endl;
			}
		    }
		}
	    }
	    for(int i = 0; i < Outputs.size(); i++) {
		bool IsInput = false;
		for(int j = 0; j < N->Inputs.size() && !IsInput; j++)
		    if(N->Inputs[j] == Outputs[i]) IsInput = true;
		if(!IsInput) {
		    if(StructureCompatible(N, Outputs[i])) {
			//if(print) { cout<<"     "<<Outputs[i]<<"\t"; PrintInterval(Outputs[i]->ON, Outputs[i]->OFF); cout<<"\t"; }
			//if(trace) { status<<"     "<<Outputs[i]<<"\t"; PrintInterval(Outputs[i]->ON, Outputs[i]->OFF, status); status<<"\t"; }
			if(OffSetCompatible(N, Outputs[i])) {
			    //if(print) cout<<"yes\t";
			    //if(trace) status<<"yes\t";
			    if(OnSetCompatible(N, Outputs[i])) {
				//if(print) cout<<"yes\t";
				//if(trace) status<<"yes\t";
				BDD totalcover;
				if(GateType == 1) totalcover = ToCover * !Outputs[i]->ON;
				else if(GateType == 2) totalcover = ToCover * !Outputs[i]->OFF;
				else if(GateType == 3) {
				    if(N->LogicOp == 'A') totalcover = ToCover * !Outputs[i]->ON;
				    else if(N->LogicOp == 'O') totalcover = ToCover * !Outputs[i]->OFF;
				    else if(N->LogicOp == 'N') totalcover = ToCover * !Outputs[i]->ON;
				} else if(GateType == 4) {
				    if(N->LogicOp == 'A') totalcover = ToCover * !Outputs[i]->ON;
				    else if(N->LogicOp == 'O') totalcover = ToCover * !Outputs[i]->OFF;
				} else if(GateType == 5) {
				  if(N->LogicOp == 'D') totalcover = ToCover * !Outputs[i]->ON;
				  else if(N->LogicOp == 'R') totalcover = ToCover * !Outputs[i]->OFF;
				}
				//if(print) { PrintFunction(totalcover); cout<<endl; }
				//if(trace) { PrintFunction(totalcover,status); status<<endl; }
				if(totalcover != ZERO)
				    CoverNodes.push_back(Outputs[i]);
			    } else {
				//if(print) cout<<"no"<<endl;
				//if(trace) status<<"no"<<endl;
			    }
			} else {
			    //if(print) cout<<"no"<<endl;
			    //if(trace) status<<"no"<<endl;
			}
		    }
		}
	    }
	}
    }
    if(NetworkOrder == 'E' || NetworkOrder == 'F') {
	if(N->Inputs.size() < FaninBound) {
	    for(int i = 0; i < Inputs.size(); i++) {
		if(StructureCompatible(N, Inputs[i])) {
		    //if(print) { cout<<"     "<<Inputs[i]<<"\t"; PrintInterval(Inputs[i]->ON, Inputs[i]->OFF); cout<<"\t"; }
		    //if(trace) { status<<"     "<<Inputs[i]<<"\t"; PrintInterval(Inputs[i]->ON, Inputs[i]->OFF, status); status<<"\t"; }
		    if(OffSetCompatible(N, Inputs[i])) {
			//if(print) cout<<"yes\t";
			//if(trace) status<<"yes\t";
			if(OnSetCompatible(N, Inputs[i])) {
			    //if(print) cout<<"yes\t";
			    //if(trace) status<<"yes\t";
			    BDD totalcover;
			    if(GateType == 1) totalcover = ToCover * !Inputs[i]->ON;
			    else if(GateType == 2) totalcover = ToCover * !Inputs[i]->OFF;
			    else if(GateType == 3) {
				if(N->LogicOp == 'A') totalcover = ToCover * !Inputs[i]->ON;
				else if(N->LogicOp == 'O') totalcover = ToCover * !Inputs[i]->OFF;
				else if(N->LogicOp == 'N') totalcover = ToCover * !Inputs[i]->ON;
			    } else if(GateType == 4) {
				if(N->LogicOp == 'A') totalcover = ToCover * !Inputs[i]->ON;
				else if(N->LogicOp == 'O') totalcover = ToCover * !Inputs[i]->OFF;
			    } else if(GateType == 5) {
			      if(N->LogicOp == 'D') totalcover = ToCover * !Inputs[i]->ON;
			      else if(N->LogicOp == 'R') totalcover = ToCover * !Inputs[i]->OFF;
			    }
			    //if(print) { PrintFunction(totalcover); cout<<endl; }
			    //if(trace) { PrintFunction(totalcover,status); status<<endl; }
			    if(totalcover != ZERO)
				CoverNodes.push_back(Inputs[i]);
			} else {
			    //if(print) cout<<"no"<<endl;
			    //if(trace) status<<"no"<<endl;
			}
		    } else {
			//if(print) cout<<"no"<<endl;
			//if(trace) status<<"no"<<endl;
		    }
		}
	    }
	}
	for(int i = 0; i < N->Inputs.size(); i++) {
	    Node* C = N->Inputs[i];
	    if(!C->IsInput()) {
		if(StructureCompatible(N, C)) {
		    //if(print) { cout<<"     "<<C<<"\t"; PrintInterval(C->ON, C->OFF); cout<<"\t"; } 
		    //if(trace) { status<<"     "<<C<<"\t"; PrintInterval(C->ON, C->OFF, status); status<<"\t"; } 
		    if(OffSetCompatible(N, C)) {
			//if(print) cout<<"yes\t";
			//if(trace) status<<"yes\t";
			if(OnSetCompatible(N, C)) {
			    //if(print) cout<<"yes\t";
			    //if(trace) status<<"yes\t";
			    BDD totalcover;
			    if(GateType == 1) totalcover = ToCover * !C->ON;
			    else if(GateType == 2) totalcover = ToCover * !C->OFF;
			    else if(GateType == 3) {
				if(N->LogicOp == 'A') totalcover = ToCover * !C->ON;
				else if(N->LogicOp == 'O') totalcover = ToCover * !C->OFF;
				else if(N->LogicOp == 'N') totalcover = ToCover * !C->ON;
			    } else if(GateType == 4) {
				if(N->LogicOp == 'A') totalcover = ToCover * !C->ON;
				else if(N->LogicOp == 'O') totalcover = ToCover * !C->OFF;
			    } else if(GateType == 5) {
			      if(N->LogicOp == 'D') totalcover = ToCover * !C->ON;
			      else if(N->LogicOp == 'R') totalcover = ToCover * !C->OFF;
			    }
			    //if(print) { PrintFunction(totalcover); cout<<endl; }
			    //if(trace) { PrintFunction(totalcover,status); status<<endl; }
			    if(totalcover != ZERO)
				CoverNodes.push_back(C);
			} else {
			    //if(print) cout<<"no"<<endl;
			    //if(trace) status<<"no"<<endl;
			}
		    } else {
			//if(print) cout<<"no"<<endl;
			//if(trace) status<<"no"<<endl;
		    }
		}
	    }
	}
	for(int i = 0; i < Internal.size(); i++) {
	    bool IsInput = false;
	    for(int j = 0; j < N->Inputs.size() && !IsInput; j++)
		if(N->Inputs[j] == Internal[i]) IsInput = true;
	    if(!IsInput) {
		if(StructureCompatible(N, Internal[i])) {
		    //if(print) { cout<<"     "<<Internal[i]<<"\t"; PrintInterval(Internal[i]->ON, Internal[i]->OFF); cout<<"\t"; }
		    //if(trace) { status<<"     "<<Internal[i]<<"\t"; PrintInterval(Internal[i]->ON, Internal[i]->OFF, status); status<<"\t"; }
		    if(OffSetCompatible(N, Internal[i])) {
			//if(print) cout<<"yes\t";
			//if(trace) status<<"yes\t";
			if(OnSetCompatible(N, Internal[i])) {
			    //if(print) cout<<"yes\t";
			    //if(trace) status<<"yes\t";
			    BDD totalcover;
			    if(GateType == 1) totalcover = ToCover * !Internal[i]->ON;
			    else if(GateType == 2) totalcover = ToCover * !Internal[i]->OFF;
			    else if(GateType == 3) {
				if(N->LogicOp == 'A') totalcover = ToCover * !Internal[i]->ON;
				else if(N->LogicOp == 'O') totalcover = ToCover * !Internal[i]->OFF;
				else if(N->LogicOp == 'N') totalcover = ToCover * !Internal[i]->ON;
			    } else if(GateType == 4) {
				if(N->LogicOp == 'A') totalcover = ToCover * !Internal[i]->ON;
				else if(N->LogicOp == 'O') totalcover = ToCover * !Internal[i]->OFF;
			    } else if(GateType == 5) {
			      if(N->LogicOp == 'D') totalcover = ToCover * !Internal[i]->ON;
			      else if(N->LogicOp == 'R') totalcover = ToCover * !Internal[i]->OFF;
			    }
			    //if(print) { PrintFunction(totalcover); cout<<endl; }
			    //if(trace) { PrintFunction(totalcover,status); status<<endl; }
			    if(totalcover != ZERO)
				CoverNodes.push_back(Internal[i]);
			} else {
			    //if(print) cout<<"no"<<endl;
			    //if(trace) status<<"no"<<endl;
			}
		    } else {
			//if(print) cout<<"no"<<endl;
			//if(trace) status<<"no"<<endl;
		    }
		}
	    }
	}
	for(int i = 0; i < Outputs.size(); i++) {
	    bool IsInput = false;
	    for(int j = 0; j < N->Inputs.size() && !IsInput; j++)
		if(N->Inputs[j] == Outputs[i]) IsInput = true;
	    if(!IsInput) {
		if(StructureCompatible(N, Outputs[i])) {
		    //if(print) { cout<<"     "<<Outputs[i]<<"\t"; PrintInterval(Outputs[i]->ON, Outputs[i]->OFF); cout<<"\t"; }
		    //if(trace) { status<<"     "<<Outputs[i]<<"\t"; PrintInterval(Outputs[i]->ON, Outputs[i]->OFF, status); status<<"\t"; }
		    if(OffSetCompatible(N, Outputs[i])) {
			//if(print) cout<<"yes\t";
			//if(trace) status<<"yes\t";
			if(OnSetCompatible(N, Outputs[i])) {
			    //if(print) cout<<"yes\t";
			    //if(trace) status<<"yes\t";
			    BDD totalcover;
			    if(GateType == 1) totalcover = ToCover * !Outputs[i]->ON;
			    else if(GateType == 2) totalcover = ToCover * !Outputs[i]->OFF;
			    else if(GateType == 3) {
				if(N->LogicOp == 'A') totalcover = ToCover * !Outputs[i]->ON;
				else if(N->LogicOp == 'O') totalcover = ToCover * !Outputs[i]->OFF;
				else if(N->LogicOp == 'N') totalcover = ToCover * !Outputs[i]->ON;
			    } else if(GateType == 4) {
				if(N->LogicOp == 'A') totalcover = ToCover * !Outputs[i]->ON;
				else if(N->LogicOp == 'O') totalcover = ToCover * !Outputs[i]->OFF;
			    } else if(GateType == 5) {
			      if(N->LogicOp == 'D') totalcover = ToCover * !Outputs[i]->ON;
			      else if(N->LogicOp == 'R') totalcover = ToCover * !Outputs[i]->OFF;
			    }
			    //if(print) { PrintFunction(totalcover); cout<<endl; }
			    //if(trace) { PrintFunction(totalcover,status); status<<endl; }
			    if(totalcover != ZERO)
				CoverNodes.push_back(Outputs[i]);
			} else {
			    //if(print) cout<<"no"<<endl;
			    //if(trace) status<<"no"<<endl;
			}
		    } else {
			//if(print) cout<<"no"<<endl;
			//if(trace) status<<"no"<<endl;
		    }
		}
	    }
	}
	if(CoverNodes.size() > 1) Randomize(CoverNodes);
    }
    //if(print) cout<<"--------------------------------------------------------------------------------------------"<<endl;
    //if(trace) status<<"--------------------------------------------------------------------------------------------"<<endl;
    return CoverNodes;
}

/* !!! */
//Returns true if A is an Ancestors of C
  bool Ancestor(Node* A, Node* C) {
    if(FindInput(A, C) > -1) return true;
    for(int i = 0; i < A->Inputs.size(); i++)
	if(Ancestor(A->Inputs[i], C))
	    return true;
    return false;
}
 
/* !!! */
//Only copies the non-Node dependent parts of a node
Node* CopyNode(Node* N, int idx) {
  Node* newN = new Node;
  NODECOUNT++;
  newN->idx = idx;
  newN->level = N->level;
  newN->ON = N->ON;
  newN->OFF = N->OFF;
  newN->Uncovered = N->Uncovered;
  if(MintermChoice > 'H' && MintermChoice < 'P') {
      newN->Label = N->Label;
      newN->UncoveredSet = N->UncoveredSet;
  }
  if(GateType == 3 || GateType == 4 || GateType == 5) newN->LogicOp = N->LogicOp;
  return newN;
}

/* !!! */
void PrintNode(Node* N, ostream& out) {
    if(!N->IsInput()) {
	if(GateType == 1) out<<"Nand ";
	else if(GateType == 2) out<<"Nor  ";
	else if((GateType == 3 || GateType == 4) && N->LogicOp == 'A') out<<"And  ";
	else if((GateType == 3 || GateType == 4) && N->LogicOp == 'O') out<<"Or   ";
	else if(GateType == 3 && N->LogicOp == 'N') out<<"Not  ";
	else if(GateType == 5 && N->LogicOp == 'D') out<<"Nand ";
	else if(GateType == 5 && N->LogicOp == 'R') out<<"Nor ";
    } else {
	out<<"     ";
    }
  out<<N<<"("<<N->level<<"): ";
  PrintInterval(N->ON, N->OFF, out);
  if(!N->IsInput()) {
      out<<" - ";
      if(MintermChoice < 'I' || MintermChoice > 'O') {
	  PrintUncoveredFunction(N, N->Uncovered, out);
      } else {
	  for(int i = 1; i < 5; i++) {
	      if(N->UncoveredSet[i] != ZERO) {
		  out<<"  ["<<i<<"]: "; PrintFunction(N->UncoveredSet[i], out);
	      }
	  }
      }
      out<<" - {";
      for(int i = 0; i < N->Connectible.size(); i++) {
	  if(i != 0)  out<<", ";
	  out<<N->Connectible[i];
      }
      out<<"} - ";
      out<<"{";
      for(set<Node*>::iterator i = N->AlreadyConnected.begin(); i != N->AlreadyConnected.end(); i++) {
	  if(i != N->AlreadyConnected.begin()) out<<", ";
	  out<<*i;
      }
      out<<"}";
  }
  out<<endl;
}

bool Compare(pair<Node*,BDD> pair1, pair<Node*,BDD> pair2) {
    return pair1.first->idx < pair2.first->idx;   //largest index first                       //BEST
}
/* !!! */
 bool UpdateOffSet(Node* N, BDD OffSet, Node* Prev, int indent) {
    BDD oldOFF = N->OFF;
    N->OFF += OffSet;
    if(N->ON * N->OFF != ZERO || N->ON == ONE || N->OFF == ONE) {
      if(print) { 
	cout<<"    "; for(int i = 0; i < indent; i++) cout<<" ";
	if(Prev != NULL) cout<<Prev<<" -> ";
	cout<<"Update Off Set: "<<N<<": "; PrintFunction(!oldOFF); cout<<" -> "; PrintFunction(!N->OFF); 
      }
      if(trace) { 
	status<<"    "; for(int i = 0; i < indent; i++) status<<" ";
	if(Prev != NULL) status<<Prev<<" -> ";
	status<<"Update Off Set: "<<N<<": "; PrintFunction(!oldOFF,status); status<<" -> "; PrintFunction(!N->OFF,status); 
      }
      if(print) cout<<" X"<<endl;
      if(trace) status<<" X"<<endl;
      return false;
    }
    if(oldOFF != N->OFF) {
      if(print) { 
	cout<<"    "; for(int i = 0; i < indent; i++) cout<<" ";
	if(Prev != NULL) cout<<Prev<<" -> ";
	cout<<"Update Off Set: "<<N<<": "; PrintFunction(!oldOFF); cout<<" -> "; PrintFunction(!N->OFF); 
      }
      if(trace) { 
	status<<"    "; for(int i = 0; i < indent; i++) status<<" ";
	if(Prev != NULL) status<<Prev<<" -> ";
	status<<"Update Off Set: "<<N<<": "; PrintFunction(!oldOFF,status); status<<" -> "; PrintFunction(!N->OFF,status); 
      }
      if(print) cout<<" *"<<endl;
      if(trace) status<<" *"<<endl;
      if(GateType == 1) {
	  for(set<Node*>::iterator i = N->Parents.begin(); i != N->Parents.end(); i++) {
	      if(!UpdateOnSet(*i, N->OFF, N, indent+2)) return false;
	  }
	  for(int i = 0; i < N->Inputs.size(); i++)
	      if(!UpdateOnSet(N->Inputs[i], N->OFF, N, indent+2)) return false;
      } else if(GateType == 2) {
	  for(set<Node*>::iterator i = N->Parents.begin(); i != N->Parents.end(); i++) {
	      Node* P = *i;
	      if(P->Inputs.size() == FaninBound) {
		  BDD P_On = N->OFF;
		  vector<BDD> S_On(FaninBound, N->OFF * P->OFF);
		  for(int j = 0; j < P->Inputs.size(); j++) {
		      Node* S = P->Inputs[j];
		      if(S != N) {
			  P_On *= S->OFF;
			  for(int k = 0; k < P->Inputs.size(); k++)
			      if(k != j) S_On[k] *= P->Inputs[k]->OFF;
		      }
		  }
		  if(!UpdateOnSet(P, P_On, N, indent+2)) return false;
		  for(int j = 0; j < P->Inputs.size(); j++) {
		      Node* S = P->Inputs[j];
		      if(S != N) {
			  if(!UpdateOnSet(S, S_On[j], N, indent+2)) return false;
		      }
		  }
	      }
	  }
	  if(N->Inputs.size() == FaninBound) {
	      for(int i = 0; i < N->Inputs.size(); i++) {
		  BDD Cover = N->OFF;
		  for(int j = 0; j < N->Inputs.size(); j++)
		      if(N->Inputs[j] != N->Inputs[i]) Cover *= N->Inputs[j]->OFF;
		  if(Cover != ZERO)
		      if(!UpdateOnSet(N->Inputs[i], Cover, N, indent+2)) return false;
	      }
	  }
      } else if(GateType == 3) {
	  //And(P):  ON(P) <- ON(N) * ON(S)       OFF(P) <- OFF(N)
	  //        OFF(S) <- ON(N) * OFF(P) 
	  //Or(P):   ON(P) <- ON(N)               OFF(P) <- OFF(N) * OFF(S) 
	  //                                       ON(S) <- OFF(N) * ON(P)
	  //Not(P):  OFF(P) <- ON(N)               ON(P) <- OFF(N)          
	  for(set<Node*>::iterator i = N->Parents.begin(); i != N->Parents.end(); i++) {
	      Node* P = *i;
	      if(P->LogicOp == 'A') {
		  if(!UpdateOffSet(P, N->OFF, N, indent+2)) return false;
	      } else if(P->LogicOp == 'N') {
		  if(!UpdateOnSet(P, N->OFF, N, indent+2)) return false;
	      } else if(P->LogicOp == 'O') {
		  if(P->Inputs.size() == FaninBound) {
		      BDD P_Off = N->OFF;
		      vector<BDD> S_On(FaninBound, N->OFF * P->ON);
		      for(int j = 0; j < P->Inputs.size(); j++) {
			  Node* S = P->Inputs[j];
			  if(S != N) {
			      P_Off *= S->OFF;
			      for(int k = 0; k < P->Inputs.size(); k++)
				  if(k != j) S_On[k] *= P->Inputs[k]->OFF;
			  }
		      }
		      if(!UpdateOffSet(P, P_Off, N, indent+2)) return false;
		      for(int j = 0; j < P->Inputs.size(); j++) {
			  Node* S = P->Inputs[j];
			  if(S != N) {
			      if(!UpdateOnSet(S, S_On[j], N, indent+2)) return false;
			  }
		      }
		  }
	      }
	  }
	  //And(N): ON(I_1) <- ON(N)            OFF(I_1) <- OFF(N) * ON(I_2)
	  //Or(N):  ON(I_1) <- ON(N) * OFF(I_2) OFF(I_1) <- OFF(N)
	  //Not(N):  OFF(I) <- ON(N)            ON(I) <- OFF(N)
	  if(N->LogicOp == 'A') {
	      if(N->Inputs.size() == FaninBound) {
		  for(int i = 0; i < N->Inputs.size(); i++) {
		      BDD Cover = N->OFF;
		      for(int j = 0; j < N->Inputs.size(); j++)
			  if(N->Inputs[j] != N->Inputs[i]) Cover *= N->Inputs[j]->ON;
		      if(Cover != ZERO)
			  if(!UpdateOffSet(N->Inputs[i], Cover, N, indent+2)) return false;
		  }
	      }
	  } else if(N->LogicOp == 'N') {
	      if(!N->Inputs.empty()) if(!UpdateOnSet(N->Inputs[0], N->OFF, N, indent+2)) return false;
	  } else if(N->LogicOp == 'O') {
	      for(int i = 0; i < N->Inputs.size(); i++)
		  if(!UpdateOffSet(N->Inputs[i], N->OFF, N, indent+2)) return false;
	  }
      } else if(GateType == 4) {
	  //And(P):  ON(P) <- ON(N) * ON(S)       OFF(P) <- OFF(N)
	  //        OFF(S) <- ON(N) * OFF(P) 
	  //Or(P):   ON(P) <- ON(N)               OFF(P) <- OFF(N) * OFF(S) 
	  //                                       ON(S) <- OFF(N) * ON(P)
	  for(set<Node*>::iterator i = N->Parents.begin(); i != N->Parents.end(); i++) {
	      Node* P = *i;
	      if(P->LogicOp == 'A') {
		  if(!UpdateOffSet(P, N->OFF, N, indent+2)) return false;
	      } else if(P->LogicOp == 'O') {
		  if(P->Inputs.size() == FaninBound) {
		      BDD P_Off = N->OFF;
		      vector<BDD> S_On(FaninBound, N->OFF * P->ON);
		      for(int j = 0; j < P->Inputs.size(); j++) {
			  Node* S = P->Inputs[j];
			  if(S != N) {
			      P_Off *= S->OFF;
			      for(int k = 0; k < P->Inputs.size(); k++)
				  if(k != j) S_On[k] *= P->Inputs[k]->OFF;
			  }
		      }
		      if(!UpdateOffSet(P, P_Off, N, indent+2)) return false;
		      for(int j = 0; j < P->Inputs.size(); j++) {
			  Node* S = P->Inputs[j];
			  if(S != N) {
			      if(!UpdateOnSet(S, S_On[j], N, indent+2)) return false;
			  }
		      }
		  }
	      }
	  }
	  //And(N): ON(I_1) <- ON(N)            OFF(I_1) <- OFF(N) * ON(I_2)
	  //Or(N):  ON(I_1) <- ON(N) * OFF(I_2) OFF(I_1) <- OFF(N)
	  if(N->LogicOp == 'A') {
	      if(N->Inputs.size() == FaninBound) {
		  for(int i = 0; i < N->Inputs.size(); i++) {
		      BDD Cover = N->OFF;
		      for(int j = 0; j < N->Inputs.size(); j++)
			  if(N->Inputs[j] != N->Inputs[i]) Cover *= N->Inputs[j]->ON;
		      if(Cover != ZERO)
			  if(!UpdateOffSet(N->Inputs[i], Cover, N, indent+2)) return false;
		  }
	      }
	  } else if(N->LogicOp == 'O') {
	      for(int i = 0; i < N->Inputs.size(); i++)
		  if(!UpdateOffSet(N->Inputs[i], N->OFF, N, indent+2)) return false;
	  }
      } else if(GateType == 5) {
	  for(set<Node*>::iterator i = N->Parents.begin(); i != N->Parents.end(); i++) {
	      Node* P = *i;
	      if(P->LogicOp == 'D') {
		  if(!UpdateOnSet(P, N->OFF, N, indent+2)) return false;
	      } else if(P->LogicOp == 'O') {
		  if(P->Inputs.size() == FaninBound) {
		      BDD P_On = N->OFF;
		      vector<BDD> S_On(FaninBound, N->OFF * P->OFF);
		      for(int j = 0; j < P->Inputs.size(); j++) {
			  Node* S = P->Inputs[j];
			  if(S != N) {
			      P_On *= S->OFF;
			      for(int k = 0; k < P->Inputs.size(); k++)
				  if(k != j) S_On[k] *= P->Inputs[k]->OFF;
			  }
		      }
		      if(!UpdateOnSet(P, P_On, N, indent+2)) return false;
		      for(int j = 0; j < P->Inputs.size(); j++) {
			  Node* S = P->Inputs[j];
			  if(S != N) {
			      if(!UpdateOnSet(S, S_On[j], N, indent+2)) return false;
			  }
		      }
		  }
	      }
	  }
	  if(N->LogicOp == 'D') {
	      for(int i = 0; i < N->Inputs.size(); i++)
		  if(!UpdateOnSet(N->Inputs[i], N->OFF, N, indent+2)) return false;
	  } else if(N->LogicOp == 'R') {
	      if(N->Inputs.size() == FaninBound) {
		  for(int i = 0; i < N->Inputs.size(); i++) {
		      BDD Cover = N->OFF;
		      for(int j = 0; j < N->Inputs.size(); j++)
			  if(N->Inputs[j] != N->Inputs[i]) Cover *= N->Inputs[j]->OFF;
		      if(Cover != ZERO)
			  if(!UpdateOnSet(N->Inputs[i], Cover, N, indent+2)) return false;
		  }
	      }
	  }
      }
    }
    return true;
 }
/* !!! */
bool UpdateOnSet(Node* N, BDD OnSet, Node* Prev, int indent) { 
  BDD oldON = N->ON;
  N->ON += OnSet;
  if(N->ON * N->OFF != ZERO || N->ON == ONE || N->OFF == ONE) {
    if(print) { 
      cout<<"    "; for(int i = 0; i < indent; i++) cout<<" ";
      if(Prev != NULL) cout<<Prev<<" -> ";
      cout<<"Update On Set: "<<N<<": "; PrintFunction(oldON); cout<<" -> "; PrintFunction(N->ON);
    }
    if(trace) { 
      status<<"    "; for(int i = 0; i < indent; i++) status<<" ";
      if(Prev != NULL) status<<Prev<<" -> ";
      status<<"Update On Set: "<<N<<": "; PrintFunction(oldON,status); status<<" -> "; PrintFunction(N->ON,status); 
    }
    if(print) cout<<" X"<<endl;	
    if(trace) status<<" X"<<endl;
    return false;
  }
  if(oldON != N->ON) { 
    if(print) { 
      cout<<"    "; for(int i = 0; i < indent; i++) cout<<" ";
      if(Prev != NULL) cout<<Prev<<" -> ";
      cout<<"Update On Set: "<<N<<": "; PrintFunction(oldON); cout<<" -> "; PrintFunction(N->ON);
    }
    if(trace) { 
      status<<"    "; for(int i = 0; i < indent; i++) status<<" ";
      if(Prev != NULL) status<<Prev<<" -> ";
      status<<"Update On Set: "<<N<<": "; PrintFunction(oldON,status); status<<" -> "; PrintFunction(N->ON,status); 
    }
    if(print) cout<<" *"<<endl;
    if(trace) status<<" *"<<endl;
    if(GateType == 1) {
	for(set<Node*>::iterator i = N->Parents.begin(); i != N->Parents.end(); i++) {
	    Node* P = *i;
	    if(P->Inputs.size() == FaninBound) {
		BDD P_Off = N->ON;
		vector<BDD> S_Off(FaninBound, N->ON * P->ON);
		for(int j = 0; j < P->Inputs.size(); j++) {
		    Node* S = P->Inputs[j];
		    if(S != N) {
			P_Off *= S->ON;
			for(int k = 0; k < P->Inputs.size(); k++)
			    if(k != j) S_Off[k] *= P->Inputs[k]->ON;
		    }
		}
		if(!UpdateOffSet(P, P_Off, N, indent+2)) return false;
		for(int j = 0; j < P->Inputs.size(); j++) {
		    Node* S = P->Inputs[j];
		    if(S != N) {
			if(!UpdateOffSet(S, S_Off[j], N, indent+2)) return false;
		    }
		}
	    }
	}
	if(N->Inputs.size() == FaninBound) {
	    for(int i = 0; i < N->Inputs.size(); i++) {
		BDD Cover = N->ON;
		for(int j = 0; j < N->Inputs.size(); j++)
		    if(N->Inputs[j] != N->Inputs[i]) Cover *= N->Inputs[j]->ON;
		if(Cover != ZERO)
		    if(!UpdateOffSet(N->Inputs[i], Cover, N, indent+2)) return false;
	    }
	}
    } else if(GateType == 2) {
	for(set<Node*>::iterator i = N->Parents.begin(); i != N->Parents.end(); i++) {
	    if(!UpdateOffSet(*i, N->ON, N, indent+2)) return false;
	}
	for(int i = 0; i < N->Inputs.size(); i++)
	    if(!UpdateOffSet(N->Inputs[i], N->ON, N, indent+2)) return false;
    } else if(GateType == 3) {
	//And(P):  ON(P) <- ON(N) * ON(S)       OFF(P) <- OFF(N)
	//        OFF(S) <- ON(N) * OFF(P) 
	//Or(P):   ON(P) <- ON(N)               OFF(P) <- OFF(N) * OFF(S) 
	//                                       ON(S) <- OFF(N) * ON(P)
	//Not(P):  OFF(P) <- ON(N)               ON(P) <- OFF(N)          
	for(set<Node*>::iterator i = N->Parents.begin(); i != N->Parents.end(); i++) {
	    Node* P = *i;
	    if(P->LogicOp == 'O') {
		if(!UpdateOnSet(P, N->ON, N, indent+2)) return false;
	    } else if(P->LogicOp == 'N') {
		if(!UpdateOffSet(P, N->ON, N, indent+2)) return false;
	    } else if(P->LogicOp == 'A') {
		if(P->Inputs.size() == FaninBound) {
		    BDD P_On = N->ON;
		    vector<BDD> S_Off(FaninBound, N->ON * P->OFF);
		    for(int k = 0; k < P->Inputs.size(); k++) {
			Node* S = P->Inputs[k];
			if(S != N) {
			    P_On *= S->ON;
			    for(int l = 0; l < P->Inputs.size(); l++)
				if(l != k) S_Off[k] *= P->Inputs[l]->ON;
			}
		    }
		    if(!UpdateOnSet(P, P_On, N, indent+2)) return false;
		    for(int j = 0; j < P->Inputs.size(); j++) {
			Node* S = P->Inputs[j];
			if(S != N) {
			    if(!UpdateOffSet(S, S_Off[j], N, indent+2)) return false;
			}
		    }
		}
	    }
	}
	//And(N): ON(I_1) <- ON(N)            OFF(I_1) <- OFF(N) * ON(I_2)
	//Or(N):  ON(I_1) <- ON(N) * OFF(I_2) OFF(I_1) <- OFF(N)
	//Not(N):  OFF(I) <- ON(N)            ON(I) <- OFF(N)
	if(N->LogicOp == 'O') {
	    if(N->Inputs.size() == FaninBound) {
		for(int i = 0; i < N->Inputs.size(); i++) {
		    BDD Cover = N->ON;
		    for(int j = 0; j < N->Inputs.size(); j++)
			if(N->Inputs[j] != N->Inputs[i]) Cover *= N->Inputs[j]->OFF;
		    if(Cover != ZERO)
			if(!UpdateOnSet(N->Inputs[i], Cover, N, indent+2)) return false;
		}
	    }
	} else if(N->LogicOp == 'N') {
	    if(!N->Inputs.empty()) if(!UpdateOffSet(N->Inputs[0], N->ON, N, indent+2)) return false;
	} else if(N->LogicOp == 'A') {
	    for(int i = 0; i < N->Inputs.size(); i++)
		if(!UpdateOnSet(N->Inputs[i], N->ON, N, indent+2)) return false;
	}
    } else if(GateType == 4) {
      //And(P):  ON(P) <- ON(N) * ON(S)       OFF(P) <- OFF(N)
      //        OFF(S) <- ON(N) * OFF(P) 
      //Or(P):   ON(P) <- ON(N)               OFF(P) <- OFF(N) * OFF(S) 
      //                                       ON(S) <- OFF(N) * ON(P)
      for(set<Node*>::iterator i = N->Parents.begin(); i != N->Parents.end(); i++) {
	Node* P = *i;
	if(P->LogicOp == 'O') {
	  if(!UpdateOnSet(P, N->ON, N, indent+2)) return false;
	} else if(P->LogicOp == 'A') {
	  if(P->Inputs.size() == FaninBound) {
	    BDD P_On = N->ON;
	    vector<BDD> S_Off(FaninBound, N->ON * P->OFF);
	    for(int k = 0; k < P->Inputs.size(); k++) {
	      Node* S = P->Inputs[k];
	      if(S != N) {
		P_On *= S->ON;
		for(int l = 0; l < P->Inputs.size(); l++)
		  if(l != k) S_Off[k] *= P->Inputs[l]->ON;
	      }
	    }
	    if(!UpdateOnSet(P, P_On, N, indent+2)) return false;
	    for(int j = 0; j < P->Inputs.size(); j++) {
	      Node* S = P->Inputs[j];
	      if(S != N) {
		if(!UpdateOffSet(S, S_Off[j], N, indent+2)) return false;
	      }
	    }
	  }
	}
      }
      //And(N): ON(I_1) <- ON(N)            OFF(I_1) <- OFF(N) * ON(I_2)
      //Or(N):  ON(I_1) <- ON(N) * OFF(I_2) OFF(I_1) <- OFF(N)
      if(N->LogicOp == 'O') {
	if(N->Inputs.size() == FaninBound) {
	  for(int i = 0; i < N->Inputs.size(); i++) {
	    BDD Cover = N->ON;
	    for(int j = 0; j < N->Inputs.size(); j++)
	      if(N->Inputs[j] != N->Inputs[i]) Cover *= N->Inputs[j]->OFF;
	    if(Cover != ZERO)
	      if(!UpdateOnSet(N->Inputs[i], Cover, N, indent+2)) return false;
	  }
	}
      } else if(N->LogicOp == 'A') {
	for(int i = 0; i < N->Inputs.size(); i++)
	  if(!UpdateOnSet(N->Inputs[i], N->ON, N, indent+2)) return false;
      }
    } else if(GateType == 5) {
      for(set<Node*>::iterator i = N->Parents.begin(); i != N->Parents.end(); i++) {
	Node* P = *i;
	if(P->LogicOp == 'D') {
	  if(P->Inputs.size() == FaninBound) {
	    BDD P_Off = N->ON;
	    vector<BDD> S_Off(FaninBound, N->ON * P->ON);
	    for(int k = 0; k < P->Inputs.size(); k++) {
	      Node* S = P->Inputs[k];
	      if(S != N) {
		P_Off *= S->ON;
		for(int l = 0; l < P->Inputs.size(); l++)
		  if(l != k) S_Off[k] *= P->Inputs[l]->ON;
	      }
	    }
	    if(!UpdateOffSet(P, P_Off, N, indent+2)) return false;
	    for(int j = 0; j < P->Inputs.size(); j++) {
	      Node* S = P->Inputs[j];
	      if(S != N) {
		if(!UpdateOffSet(S, S_Off[j], N, indent+2)) return false;
	      }
	    }
	  }
	} else if(P->LogicOp == 'R') {
	  if(!UpdateOffSet(P, N->ON, N, indent+2)) return false;
	  
	}
	if(N->LogicOp == 'D') {
	    if(N->Inputs.size() == FaninBound) {
		for(int i = 0; i < N->Inputs.size(); i++) {
		    BDD Cover = N->ON;
		    for(int j = 0; j < N->Inputs.size(); j++)
			if(N->Inputs[j] != N->Inputs[i]) Cover *= N->Inputs[j]->ON;
		    if(Cover != ZERO)
			if(!UpdateOffSet(N->Inputs[i], Cover, N, indent+2)) return false;
		}
	    }
	} else if(N->LogicOp == 'R') {
	    for(int i = 0; i < N->Inputs.size(); i++)
		if(!UpdateOffSet(N->Inputs[i], N->ON, N, indent+2)) return false;
	}
      }
    }
  }
  return true;
}

//////////////////////////////////////////////////////////////////////////////
// BDD Functions
//////////////////////////////////////////////////////////////////////////////
/*int MintermRank(BDD M) {
    int ct = 0;
    for(int i = 0; i < InputBdds.size(); i++) {
	if((GateType == 1 && !InputBdds[i] * M == M) ||
	   (GateType == 2 && InputBdds[i] * M == M)) ct++;
    }
    return ct;
    }*/
/* !!! */
bool BddLeq(BDD f, BDD g) {
  return ((f & !g) == ZERO);
}

/* !!! */
int Number(BDD F) {
  if(INPUT > 5) return 0;
  int sol = 0;
  int Total = (1<<INPUT);
  for(int i = 0; i < Total; i++) {
    BDD M = MakeMinterm(InputBdds, i);
    if(F.Constrain(M) == ONE) sol += 1<<(Total-i-1);
  }
  return sol;
}

int MintermIndex(BDD M) {
  int index = 0;
  for(int i = 0; i < InputBdds.size(); i++) {
    if(M * InputBdds[INPUT-i-1] == M) index += (1<<i);
  }
  return index;
}
bool EmptyIntersection(vector<Node*>& A, vector<Node*>& B) {
    for(int i = 0; i < A.size(); i++) {
	for(int j = 0; j < B.size(); j++) {
	    if(A[i] == B[j]) return false;
	}
    }
    return true;
}

BDD Network::SelectMintermForCovering(Node*& N, vector<Node*>& Connectible) {
    if(print) cout<<"Select Minterm for Covering"<<endl;
    if(GateType == 4 && FaninBound == INF) {
	DdNode* inputnodes[INPUT];
	for(int i = 0; i < INPUT; i++)
	    inputnodes[i] = InputBdds[i].getNode();
	BDDvector InputVector(INPUT, Manager, inputnodes);

	//Find the set of minterms with fewest covering options	
	int MinCovering = INF;
	vector<pair<Node*,BDD> > Minterms;
	for(int i = 0; i < Outputs.size(); i++) {
	    if(Outputs[i]->Uncovered != ZERO) {
		BDD TempCover = Outputs[i]->Uncovered;
		while(TempCover != ZERO) {
		    BDD M = TempCover.PickOneMinterm(InputVector);
		    int ct = 0;
		    for(int j = 0; j < Outputs[i]->Connectible.size(); j++)
			if((Outputs[i]->LogicOp == 'A' && !Outputs[i]->Connectible[j]->ON * M != ZERO) ||
			   (Outputs[i]->LogicOp == 'O' && !Outputs[i]->Connectible[j]->OFF * M != ZERO))
			    ct++;
		    if(ct < MinCovering) { 
			MinCovering = ct;
			Minterms.clear();
			Minterms.push_back(pair<Node*,BDD> (Outputs[i], M));
		    } else if(ct == MinCovering) { 
			Minterms.push_back(pair<Node*,BDD> (Outputs[i], M));
		    }
		    TempCover *= !M;
		}
	    }
	}
	for(int i = 0; i < Internal.size(); i++) {
	    if(Internal[i]->Uncovered != ZERO) {
		BDD TempCover = Internal[i]->Uncovered;
		while(TempCover != ZERO) {
		    BDD M = TempCover.PickOneMinterm(InputVector);
		    bool T = false;
		    int ct = 0;
		    for(int j = 0; j < Internal[i]->Connectible.size(); j++)
			if((Internal[i]->LogicOp == 'A' && !Internal[i]->Connectible[j]->ON * M != ZERO) ||
			   (Internal[i]->LogicOp == 'O' && !Internal[i]->Connectible[j]->OFF * M != ZERO))
			    ct++;
		    if(ct < MinCovering) { 
			MinCovering = ct;
			Minterms.clear();
			Minterms.push_back(pair<Node*,BDD> (Internal[i], M));
		    } else if(ct == MinCovering) { 
			Minterms.push_back(pair<Node*,BDD> (Internal[i], M));
		    }
		    TempCover *= !M;
		}
	    }
	}
	if(print) cout<<"  "<<Minterms.size()<<" minterms with "<<MinCovering<<" covers."<<endl;
	BDD M;
	if(Minterms.size() == 1 || MinCovering == 0) {
	    M = Minterms[0].second;
	    N = Minterms[0].first;
	} else {
	    //Find the minterm with most difficult covering type
	    int maxLabel = 0;
	    if(print) cout<<"  node, minterm: label"<<endl;
	    for(int i = 0; i < Minterms.size(); i++) {
		Node* tempN = Minterms[i].first;
		BDD tempM = Minterms[i].second;
		int tempL = GetLabel(tempN, tempM);
		if(print) { cout<<"  "<<tempN<<", "; PrintFunction(tempM); cout<<": "<<tempL<<endl; }
		if(tempL > maxLabel) {
		    M = tempM;
		    N = tempN;
		    maxLabel = tempL;
		}
	    }
	}	
	if(print) { cout<<"  Node = "<<N<<endl<<"  Minterm = "; PrintFunction(M); cout<<endl; }
	//Get Connectible Set
	vector<vector<Node*> > ConnectibleOrder(7);
	for(int k = 0; k < N->Connectible.size(); k++) {
	    Node* C = N->Connectible[k];
	    if((N->LogicOp == 'A' && C->ON * M == ZERO) ||
	       (N->LogicOp == 'O' && C->OFF * M == ZERO)) {
		//1: C is primary input
		if(C->IsInput())
		    ConnectibleOrder[1].push_back(C);
		else {
		    bool IsFanin = false;
		    for(int i = 0; i < N->Inputs.size() && !IsFanin; i++)
			if(N->Inputs[i] == C) IsFanin = true;
		    //0, 4: C is fan-in of N
		    if(IsFanin) {
			//0: C has opposite Logic Op from N
			if(C->LogicOp != N->LogicOp)
			    ConnectibleOrder[0].push_back(C);
			//4: C has same Logic Op as N
			else
			    ConnectibleOrder[4].push_back(C);
		    } else {
			//2, 3: C has opposite Logic Op from N
			if(C->LogicOp != N->LogicOp) {
			    //2: C contains M in on/off-set
			    if((N->LogicOp == 'A' && C->OFF * M != ZERO) ||
			       (N->LogicOp == 'O' && C->ON * M != ZERO))
				ConnectibleOrder[2].push_back(C);
			    //3: C contains M in dc-set
			    else
				ConnectibleOrder[3].push_back(C);
			//5, 6: C has same Logic Op as N
			} else {
			    //5: C contains M in on/off-set
			    if((N->LogicOp == 'A' && C->OFF * M != ZERO) ||
			       (N->LogicOp == 'O' && C->ON * M != ZERO))
				ConnectibleOrder[2].push_back(C);
			    //6: C contains M in dc-set
			    else
				ConnectibleOrder[3].push_back(C);
			}
		    }
		    //Connectible.push_back(N->Connectible[k]);
		}
	    }
	}
	Connectible.clear();
	for(int i = 0; i < 7; i++) {
	    for(int j = 0; j < ConnectibleOrder[i].size(); j++)
		Connectible.push_back(ConnectibleOrder[i][j]);
	}
	return M;
    }
    //A: (1) Randomly Select a Minterm
    if(MintermChoice == 'A') {
	int countMinterms = 0;
	map<int, pair<Node*,int> > MintermIndex;
	map<int, BDD> ToCoverIndex;
	//Find all uncovered minterms
	for(int i = 0; i < Outputs.size(); i++) {
	    if(Outputs[i]->Uncovered != ZERO) {
		double Mcount = Outputs[i]->Uncovered.CountMinterm(INPUT);
		MintermIndex[countMinterms] = pair<Node*, int> (Outputs[i], countMinterms+(int)Mcount);
		ToCoverIndex[countMinterms] = Outputs[i]->Uncovered;
		countMinterms += (int)Mcount;
	    }
	}
	for(int i = 0; i < Internal.size(); i++) {
	    if(Internal[i]->Uncovered != ZERO) {
		double Mcount = Internal[i]->Uncovered.CountMinterm(INPUT);
		MintermIndex[countMinterms] = pair<Node*, int> (Internal[i], countMinterms+(int)Mcount);
		ToCoverIndex[countMinterms] = Internal[i]->Uncovered;
		countMinterms += (int)Mcount;
	    }
	}
	//Randomly select the index for one of the minterms
	double r = (double)rand() / ((double)(RAND_MAX)+(double)(1));
	double x = r * countMinterms;
	int index = (int)x + 1;
	//Find the index in the intervals
	for(map<int,pair<Node*,int> >::iterator i = MintermIndex.begin(); i != MintermIndex.end(); i++) {
	    if((*i).first < index && index <= (*i).second.second) {
		BDD f = ToCoverIndex[(*i).first];
		int counter = index - (*i).first;
		int pwr = 1<<INPUT;
		for(int j = 0; j < pwr; j++) {
		    BDD M = MakeMinterm(InputBdds, j);
		    if(f * M != ZERO) {
			counter--;
			if(counter == 0) {
			    //Found minterm
			    N = (*i).second.first;
			    //Get Connectible Set
			    Connectible.clear();
			    for(int k = 0; k < N->Connectible.size(); k++)
				if((GateType == 1 && N->Connectible[k]->ON * M == ZERO) ||
				   (GateType == 2 && N->Connectible[k]->OFF * M == ZERO) ||
				   ((GateType == 3 || GateType == 4) && N->LogicOp == 'A' && N->Connectible[k]->ON * M == ZERO) ||
				   ((GateType == 3 || GateType == 4) && N->LogicOp == 'O' && N->Connectible[k]->OFF * M == ZERO) ||
				   (GateType == 3 && N->LogicOp == 'N' && N->Connectible[k]->ON * M == ZERO))
				    Connectible.push_back(N->Connectible[k]);
			    return M;
			}
		    }
		}  
	    }
	}
    }
    //B: (1) Randomly Select a Node (2) Let Cudd Pick Minterm
    if(MintermChoice == 'B') {
	//Find all uncovered nodes
	vector<Node*> NodeIndex;
	vector<BDD> ToCoverIndex;
	for(int i = 0; i < Outputs.size(); i++) {
	    if(Outputs[i]->Uncovered != ZERO) {
		NodeIndex.push_back(Outputs[i]);
		ToCoverIndex.push_back(Outputs[i]->Uncovered);
	    }
	}
	for(int i = 0; i < Internal.size(); i++) {
	    if(Internal[i]->Uncovered != ZERO) {
		NodeIndex.push_back(Internal[i]);
		ToCoverIndex.push_back(Internal[i]->Uncovered);
	    }
	}
	//Randomly Select Node
	int countNodes = NodeIndex.size();
	double r = (double)rand() / ((double)(RAND_MAX)+(double)(1));
	double x = r * countNodes;
	int index = (int)x;
	N = NodeIndex[index];
	BDD ToCover = ToCoverIndex[index];
	//Use Cudd to pick Minterm
	DdNode* inputnodes[INPUT];
	for(int i = 0; i < INPUT; i++)
	    inputnodes[i] = InputBdds[i].getNode();
	BDDvector InputVector(INPUT, Manager, inputnodes);
	BDD M = ToCover.PickOneMinterm(InputVector);
	//Get Connectible Set
	Connectible.clear();
	for(int k = 0; k < N->Connectible.size(); k++) {
	    if((GateType == 1 && N->Connectible[k]->ON * M == ZERO) ||
	       (GateType == 2 && N->Connectible[k]->OFF * M == ZERO) ||
	       ((GateType == 3 || GateType == 4) && N->LogicOp == 'A' && N->Connectible[k]->ON * M == ZERO) ||
	       ((GateType == 3 || GateType == 4) && N->LogicOp == 'O' && N->Connectible[k]->OFF * M == ZERO) ||
	       (GateType == 3 && N->LogicOp == 'N' && N->Connectible[k]->ON * M == ZERO))
		Connectible.push_back(N->Connectible[k]);
	}
	return M;
    }
    //C: Randomly select from the set of minterms with the fewest coverings options
    if(MintermChoice == 'C') {
	DdNode* inputnodes[INPUT];
	for(int i = 0; i < INPUT; i++)
	    inputnodes[i] = InputBdds[i].getNode();
	BDDvector InputVector(INPUT, Manager, inputnodes);

	//Find the set of minterms with fewest covering options	
	int MinCovering = INF;
	vector<pair<Node*,BDD> > Minterms;
	for(int i = 0; i < Outputs.size(); i++) {
	    if(Outputs[i]->Uncovered != ZERO) {
		BDD TempCover = Outputs[i]->Uncovered;
		while(TempCover != ZERO) {
		    BDD M = TempCover.PickOneMinterm(InputVector);
		    bool T = false;
		    int ct = 0;
		    for(int j = 0; j < Outputs[i]->Connectible.size(); j++)
			if(M * !Outputs[i]->Connectible[j]->ON != ZERO) ct++;    
		    if(ct < MinCovering) { 
			MinCovering = ct; 
			Minterms.clear();
			Minterms.push_back(pair<Node*,BDD> (Outputs[i], M));
		    } else if(ct == MinCovering) { 
			Minterms.push_back(pair<Node*,BDD> (Outputs[i], M));
		    }
		    TempCover *= !M;
		}
	    }
	}
	for(int i = 0; i < Internal.size(); i++) {
	    if(Internal[i]->Uncovered != ZERO) {
		BDD TempCover = Internal[i]->Uncovered;
		while(TempCover != ZERO) {
		    BDD M = TempCover.PickOneMinterm(InputVector);
		    bool T = false;
		    int ct = 0;
		    for(int j = 0; j < Internal[i]->Connectible.size(); j++)
			if(M * !Internal[i]->Connectible[j]->ON != ZERO) ct++;    
		    if(ct < MinCovering) { 
			MinCovering = ct; 
			Minterms.clear();
			Minterms.push_back(pair<Node*,BDD> (Internal[i], M));
		    } else if(ct == MinCovering) { 
			Minterms.push_back(pair<Node*,BDD> (Internal[i], M));
		    }
		    TempCover *= !M;
		}
	    }
	}
	//Randomly Select Minterm
	int total = Minterms.size();
	double r = (double)rand() / ((double)(RAND_MAX)+(double)(1));
	double x = r * total;
	int index = (int)x;
	/*if(print) {
	    cout<<"SelectMinterm "<<index<<" / "<<total<<endl;
	    cout<<"  Options("<<MinCovering<<"): ";
	    for(int i = 0; i < Minterms.size(); i++) {
		if(i != 0) cout<<", ";
		cout<<"("<<Minterms[i].first<<", "; PrintFunction(Minterms[i].second); cout<<")";
	    }
	    cout<<endl;
	    }*/
	N = Minterms[index].first;
	BDD M = Minterms[index].second;
	if(print) {
	    cout<<"  Node = "<<N<<endl;
	    cout<<"  Minterm = "; PrintFunction(M); cout<<endl;
	}
	//Get Connectible Set
	Connectible.clear();
	for(int k = 0; k < N->Connectible.size(); k++) {
	    if((GateType == 1 && N->Connectible[k]->ON * M == ZERO) ||
	       (GateType == 2 && N->Connectible[k]->OFF * M == ZERO) ||
	       ((GateType == 3 || GateType == 4) && N->LogicOp == 'A' && N->Connectible[k]->ON * M == ZERO) ||
	       ((GateType == 3 || GateType == 4) && N->LogicOp == 'O' && N->Connectible[k]->OFF * M == ZERO) ||
	       (GateType == 3 && N->LogicOp == 'N' && N->Connectible[k]->ON * M == ZERO))
		Connectible.push_back(N->Connectible[k]);
	}
	return M;
    }
    //D: Randomly select from the set of minterm with the most covering options
    if(MintermChoice == 'D') {
	DdNode* inputnodes[INPUT];
	for(int i = 0; i < INPUT; i++)
	    inputnodes[i] = InputBdds[i].getNode();
	BDDvector InputVector(INPUT, Manager, inputnodes);
	
	//Find the set of minterms with most covering options
	int MaxCovering = -1;
	vector<pair<Node*,BDD> > Minterms;
	for(int i = 0; i < Outputs.size(); i++) {
	    if(Outputs[i]->Uncovered != ZERO) {
		BDD TempCover = Outputs[i]->Uncovered;
		while(TempCover != ZERO) {
		    BDD M = TempCover.PickOneMinterm(InputVector);
		    int ct = 0;
		    for(int j = 0; j < Outputs[i]->Connectible.size(); j++)
			if((GateType == 1 && !Outputs[i]->Connectible[j]->ON * M != ZERO) ||
			   (GateType == 2 && !Outputs[i]->Connectible[j]->OFF * M != ZERO) ||
			   ((GateType == 3 || GateType == 4) && Outputs[i]->LogicOp == 'A' && !Outputs[i]->Connectible[j]->ON * M != ZERO) ||
			   ((GateType == 3 || GateType == 4) && Outputs[i]->LogicOp == 'O' && !Outputs[i]->Connectible[j]->OFF * M != ZERO) ||
			   (GateType == 3 && Outputs[i]->LogicOp == 'N' && !Outputs[i]->Connectible[j]->ON * M != ZERO))
			    ct++;
		    if(ct > MaxCovering) { 
			MaxCovering = ct; 
			Minterms.clear();
			Minterms.push_back(pair<Node*,BDD> (Outputs[i], M));
		    } else if(ct == MaxCovering) { 
			Minterms.push_back(pair<Node*,BDD> (Outputs[i], M));
		    }
		    TempCover *= !M;
		}
	    }
	}
	for(int i = 0; i < Internal.size(); i++) {
	    if(Internal[i]->Uncovered != ZERO) {
		BDD TempCover = Internal[i]->Uncovered;
		while(TempCover != ZERO) {
		    BDD M = TempCover.PickOneMinterm(InputVector);
		    int ct = 0;
		    for(int j = 0; j < Internal[i]->Connectible.size(); j++)
			if((GateType == 1 && !Internal[i]->Connectible[j]->ON * M != ZERO) ||
			   (GateType == 2 && !Internal[i]->Connectible[j]->OFF * M != ZERO) ||
			   ((GateType == 3 || GateType == 4) && Internal[i]->LogicOp == 'A' && !Internal[i]->Connectible[j]->ON * M != ZERO) ||
			   ((GateType == 3 || GateType == 4) && Internal[i]->LogicOp == 'O' && !Internal[i]->Connectible[j]->OFF * M != ZERO) ||
			   (GateType == 3 && Internal[i]->LogicOp == 'N' && !Internal[i]->Connectible[j]->ON * M != ZERO))
			    ct++;
		    if(ct > MaxCovering) { 
			MaxCovering = ct; 
			Minterms.clear();
			Minterms.push_back(pair<Node*,BDD> (Internal[i], M));
		    } else if(ct == MaxCovering) { 
			Minterms.push_back(pair<Node*,BDD> (Internal[i], M));
		    }
		    TempCover *= !M;
		}
	    }
	}
	//Randomly Select Minterm
	int total = Minterms.size();
	double r = (double)rand() / ((double)(RAND_MAX)+(double)(1));
	double x = r * total;
	int index = (int)x;
	/*if(print) {
	    cout<<"SelectMinterm "<<index<<" / "<<total<<endl;
	    cout<<"  Options("<<MaxCovering<<"): ";
	    for(int i = 0; i < Minterms.size(); i++) {
		if(i != 0) cout<<", ";
		cout<<"("<<Minterms[i].first<<", "; PrintFunction(Minterms[i].second); cout<<")";
	    }
	    cout<<endl;
	    }*/
	N = Minterms[index].first;
	BDD M = Minterms[index].second;
	if(print) { cout<<"  Node = "<<N<<endl<<"  Minterm = "; PrintFunction(M); cout<<endl; }
	//Get Connectible Set
	Connectible.clear();
	for(int k = 0; k < N->Connectible.size(); k++) {
	    if((GateType == 1 && N->Connectible[k]->ON * M == ZERO) ||
	       (GateType == 2 && N->Connectible[k]->OFF * M == ZERO) ||
	       ((GateType == 3 || GateType == 4) && N->LogicOp == 'A' && N->Connectible[k]->ON * M == ZERO) ||
	       ((GateType == 3 || GateType == 4) && N->LogicOp == 'O' && N->Connectible[k]->OFF * M == ZERO) ||
	       (GateType == 3 && N->LogicOp == 'N' && N->Connectible[k]->ON * M == ZERO))
		Connectible.push_back(N->Connectible[k]);
	}
	return M;
    }
    //E: (1) Randomly select a node from the set with the smallest fan-in (2) Let Cudd Pick a Minterm
    if(MintermChoice == 'E') {
	int minFanin = INF;
	vector<pair<Node*,BDD> > NodeSet;
	for(int i = 0; i < Outputs.size(); i++) {
	    if(Outputs[i]->Uncovered != ZERO) {
		if(Outputs[i]->Inputs.size() <= minFanin) {
		    if(Outputs[i]->Inputs.size() < minFanin) {
			minFanin = Outputs[i]->Inputs.size();
			NodeSet.clear();
		    }
		    NodeSet.push_back(pair<Node*,BDD> (Outputs[i], Outputs[i]->Uncovered));
		}
	    }
	}
	for(int i = 0; i < Internal.size(); i++) {
	    if(Internal[i]->Uncovered != ZERO) {
		if(Internal[i]->Inputs.size() <= minFanin) {
		    if(Internal[i]->Inputs.size() < minFanin) {
			minFanin = Internal[i]->Inputs.size();
			NodeSet.clear();
		    }
		    NodeSet.push_back(pair<Node*,BDD> (Internal[i], Internal[i]->Uncovered));
		}
	    }
	}
	//Randomly Select Node
	int total = NodeSet.size();
	double r = (double)rand() / ((double)(RAND_MAX)+(double)(1));
	double x = r * total;
	int index = (int)x;
	/*if(print) {
	    cout<<"SelectNode "<<index<<" / "<<total<<endl;
	    cout<<"  Options("<<minFanin<<"): ";
	    for(int i = 0; i < NodeSet.size(); i++) {
		if(i != 0) cout<<", ";
		cout<<NodeSet[i].first;
	    }
	    cout<<endl;
	    }*/
	N = NodeSet[index].first;
	BDD ToCover = NodeSet[index].second;
	if(print) { cout<<"  Node = "<<N<<endl<<"  ToCover = "; PrintFunction(ToCover); cout<<endl; }
	//Use Cudd to pick Minterm
	DdNode* inputnodes[INPUT];
	for(int i = 0; i < INPUT; i++)
	    inputnodes[i] = InputBdds[i].getNode();
	BDDvector InputVector(INPUT, Manager, inputnodes);
	BDD M = ToCover.PickOneMinterm(InputVector);
	if(print) {cout<<"  Minterm = "; PrintFunction(M); cout<<endl; }
	//Get Connectible Set
	Connectible.clear();
	for(int k = 0; k < N->Connectible.size(); k++) {
	    if((GateType == 1 && N->Connectible[k]->ON * M == ZERO) ||
	       (GateType == 2 && N->Connectible[k]->OFF * M == ZERO) ||
	       ((GateType == 3 || GateType == 4) && N->LogicOp == 'A' && N->Connectible[k]->ON * M == ZERO) ||
	       ((GateType == 3 || GateType == 4) && N->LogicOp == 'O' && N->Connectible[k]->OFF * M == ZERO) ||
	       (GateType == 3 && N->LogicOp == 'N' && N->Connectible[k]->ON * M == ZERO))
		Connectible.push_back(N->Connectible[k]);
	}
	return M;
    }
    //F: (1) Find the set of nodes with smallest fan-in 
    //   (2) Randomly select from the set of minterms with the fewest covering options
    if(MintermChoice == 'F') {
	int minFanin = INF;
	vector<pair<Node*,BDD> > NodeSet;
	for(int i = 0; i < Outputs.size(); i++) {
	    if(Outputs[i]->Uncovered != ZERO) {
		if(Outputs[i]->Inputs.size() <= minFanin) {
		    if(Outputs[i]->Inputs.size() < minFanin) {
			minFanin = Outputs[i]->Inputs.size();
			NodeSet.clear();
		    }
		    NodeSet.push_back(pair<Node*,BDD> (Outputs[i], Outputs[i]->Uncovered));
		}
	    }
	}
	for(int i = 0; i < Internal.size(); i++) {
	    if(Internal[i]->Uncovered != ZERO) {
		if(Internal[i]->Inputs.size() <= minFanin) {
		    if(Internal[i]->Inputs.size() < minFanin) {
			minFanin = Internal[i]->Inputs.size();
			NodeSet.clear();
		    }
		    NodeSet.push_back(pair<Node*,BDD> (Internal[i], Internal[i]->Uncovered));
		}
	    }
	}
	//Got Node set now get Minterm set
	DdNode* inputnodes[INPUT];
	for(int i = 0; i < INPUT; i++)
	    inputnodes[i] = InputBdds[i].getNode();
	BDDvector InputVector(INPUT, Manager, inputnodes);
	int minCovering = INF;
	vector<pair<Node*,BDD> > MintermSet;
	for(int i = 0; i < NodeSet.size(); i++) {
	    Node* node = NodeSet[i].first;
	    BDD ToCover = NodeSet[i].second;
	    while(ToCover != ZERO) {
		BDD M = ToCover.PickOneMinterm(InputVector);
		int ct = 0;
		for(int j = 0; j < node->Connectible.size(); j++)
		    if(M * !node->Connectible[j]->ON != ZERO) ct++;    
		if(ct <= minCovering) { 
		    if(ct < minCovering) {
			minCovering = ct; 
			MintermSet.clear();
		    }
		    MintermSet.push_back(pair<Node*,BDD> (node, M));
		}
		ToCover *= !M;      
	    }
	}
	//Randomly Select Minterm
	int total = MintermSet.size();
	double r = (double)rand() / ((double)(RAND_MAX)+(double)(1));
	double x = r * total;
	int index = (int)x;
	/*if(print) {
	    cout<<"Select Minterm "<<index<<" / "<<total<<endl;
	    cout<<"  Minterm Options("<<minCovering<<"): ";
	    for(int i = 0; i < MintermSet.size(); i++) {
		if(i != 0) cout<<", ";
		cout<<"("<<MintermSet[i].first<<", "; PrintFunction(MintermSet[i].second); cout<<")";
	    }
	    cout<<endl;
	    }*/
	N = MintermSet[index].first;
	BDD M = MintermSet[index].second;
	if(print) { cout<<"  Node = "<<N<<endl<<"  Minterm = "; PrintFunction(M); cout<<endl; }
	//Get Connectible Set
	Connectible.clear();
	for(int k = 0; k < N->Connectible.size(); k++) {
	    if((GateType == 1 && N->Connectible[k]->ON * M == ZERO) ||
	       (GateType == 2 && N->Connectible[k]->OFF * M == ZERO) ||
	       ((GateType == 3 || GateType == 4) && N->LogicOp == 'A' && N->Connectible[k]->ON * M == ZERO) ||
	       ((GateType == 3 || GateType == 4) && N->LogicOp == 'O' && N->Connectible[k]->OFF * M == ZERO) ||
	       (GateType == 3 && N->LogicOp == 'N' && N->Connectible[k]->ON * M == ZERO))
		Connectible.push_back(N->Connectible[k]);
	}
	return M;
    }
    //P: (1) Randomly select a node from the set with the smallest connectible set (2) Let Cudd Pick a Minterm
    if(MintermChoice == 'Q') {
	int minConn = INF;
	vector<pair<Node*,BDD> > NodeSet;
	for(int i = 0; i < Outputs.size(); i++) {
	    if(Outputs[i]->Uncovered != ZERO) {
		if(Outputs[i]->Connectible.size() <= minConn) {
		    if(Outputs[i]->Connectible.size() < minConn) {
			minConn = Outputs[i]->Connectible.size();
			NodeSet.clear();
		    }
		    NodeSet.push_back(pair<Node*,BDD> (Outputs[i], Outputs[i]->Uncovered));
		}
	    }
	}
	for(int i = 0; i < Internal.size(); i++) {
	    if(Internal[i]->Uncovered != ZERO) {
		if(Internal[i]->Connectible.size() <= minConn) {
		    if(Internal[i]->Connectible.size() < minConn) {
			minConn = Internal[i]->Connectible.size();
			NodeSet.clear();
		    }
		    NodeSet.push_back(pair<Node*,BDD> (Internal[i], Internal[i]->Uncovered));
		}
	    }
	}
	//Randomly Select Node
	int total = NodeSet.size();
	double r = (double)rand() / ((double)(RAND_MAX)+(double)(1));
	double x = r * total;
	int index = (int)x;
	if(print) {
	    cout<<"SelectNode "<<index<<" / "<<total<<endl;
	    cout<<"  Options("<<minConn<<"): ";
	    for(int i = 0; i < NodeSet.size(); i++) {
		if(i != 0) cout<<", ";
		cout<<NodeSet[i].first;
	    }
	    cout<<endl;
	}
	N = NodeSet[index].first;
	BDD ToCover = NodeSet[index].second;
	if(print) { cout<<"  Node = "<<N<<endl<<"  ToCover = "; PrintFunction(ToCover); cout<<endl; }
	//Use Cudd to pick Minterm
	DdNode* inputnodes[INPUT];
	for(int i = 0; i < INPUT; i++)
	    inputnodes[i] = InputBdds[i].getNode();
	BDDvector InputVector(INPUT, Manager, inputnodes);
	BDD M = ToCover.PickOneMinterm(InputVector);
	if(print) {cout<<"  Minterm = "; PrintFunction(M); cout<<endl; }
	//Get Connectible Set
	Connectible.clear();
	for(int k = 0; k < N->Connectible.size(); k++) {
	    if((GateType == 1 && N->Connectible[k]->ON * M == ZERO) ||
	       (GateType == 2 && N->Connectible[k]->OFF * M == ZERO) ||
	       ((GateType == 3 || GateType == 4) && N->LogicOp == 'A' && N->Connectible[k]->ON * M == ZERO) ||
	       ((GateType == 3 || GateType == 4) && N->LogicOp == 'O' && N->Connectible[k]->OFF * M == ZERO) ||
	       (GateType == 3 && N->LogicOp == 'N' && N->Connectible[k]->ON * M == ZERO))
		Connectible.push_back(N->Connectible[k]);
	}
	return M;
    }
    //Q: (1) Find the set of nodes with smallest connectible set 
    //   (2) Randomly select from the set of minterms with the fewest covering options
    if(MintermChoice == 'R') {
	int minConn = INF;
	vector<pair<Node*,BDD> > NodeSet;
	for(int i = 0; i < Outputs.size(); i++) {
	    if(Outputs[i]->Uncovered != ZERO) {
		if(Outputs[i]->Connectible.size() <= minConn) {
		    if(Outputs[i]->Connectible.size() < minConn) {
			minConn = Outputs[i]->Connectible.size();
			NodeSet.clear();
		    }
		    NodeSet.push_back(pair<Node*,BDD> (Outputs[i], Outputs[i]->Uncovered));
		}
	    }
	}
	for(int i = 0; i < Internal.size(); i++) {
	    if(Internal[i]->Uncovered != ZERO) {
		if(Internal[i]->Connectible.size() <= minConn) {
		    if(Internal[i]->Connectible.size() < minConn) {
			minConn = Internal[i]->Connectible.size();
			NodeSet.clear();
		    }
		    NodeSet.push_back(pair<Node*,BDD> (Internal[i], Internal[i]->Uncovered));
		}
	    }
	}
	//Got Node set now get Minterm set
	DdNode* inputnodes[INPUT];
	for(int i = 0; i < INPUT; i++)
	    inputnodes[i] = InputBdds[i].getNode();
	BDDvector InputVector(INPUT, Manager, inputnodes);
	int minCovering = INF;
	vector<pair<Node*,BDD> > MintermSet;
	for(int i = 0; i < NodeSet.size(); i++) {
	    Node* node = NodeSet[i].first;
	    BDD ToCover = NodeSet[i].second;
	    while(ToCover != ZERO) {
		BDD M = ToCover.PickOneMinterm(InputVector);
		int ct = 0;
		for(int j = 0; j < node->Connectible.size(); j++)
		    if(M * !node->Connectible[j]->ON != ZERO) ct++;    
		if(ct <= minCovering) { 
		    if(ct < minCovering) {
			minCovering = ct; 
			MintermSet.clear();
		    }
		    MintermSet.push_back(pair<Node*,BDD> (node, M));
		}
		ToCover *= !M;      
	    }
	}
	//Randomly Select Minterm
	int total = MintermSet.size();
	double r = (double)rand() / ((double)(RAND_MAX)+(double)(1));
	double x = r * total;
	int index = (int)x;
	/*if(print) {
	    cout<<"Select Minterm "<<index<<" / "<<total<<endl;
	    cout<<"  Minterm Options("<<minCovering<<"): ";
	    for(int i = 0; i < MintermSet.size(); i++) {
		if(i != 0) cout<<", ";
		cout<<"("<<MintermSet[i].first<<", "; PrintFunction(MintermSet[i].second); cout<<")";
	    }
	    cout<<endl;
	    }*/
	N = MintermSet[index].first;
	BDD M = MintermSet[index].second;
	if(print) { cout<<"  Node = "<<N<<endl<<"  Minterm = "; PrintFunction(M); cout<<endl; }
	//Get Connectible Set
	Connectible.clear();
	for(int k = 0; k < N->Connectible.size(); k++) {
	    if((GateType == 1 && N->Connectible[k]->ON * M == ZERO) ||
	       (GateType == 2 && N->Connectible[k]->OFF * M == ZERO) ||
	       ((GateType == 3 || GateType == 4) && N->LogicOp == 'A' && N->Connectible[k]->ON * M == ZERO) ||
	       ((GateType == 3 || GateType == 4) && N->LogicOp == 'O' && N->Connectible[k]->OFF * M == ZERO) ||
	       (GateType == 3 && N->LogicOp == 'N' && N->Connectible[k]->ON * M == ZERO))
		Connectible.push_back(N->Connectible[k]);
	}
	return M;
    }
    //G: (1) Find the node with (1) smallest fan-in, (2) largest index. 
    //   (2) Find the minterm with (1) fewest covering options, (2)  smallest index.
    if(MintermChoice == 'G') {
	int minFanin = INF;
	BDD ToCover;
	for(int i = 0; i < Outputs.size(); i++) {
	    if(Outputs[i]->Uncovered != ZERO) {
		if((Outputs[i]->Inputs.size() < minFanin) ||
		   (Outputs[i]->Inputs.size() == minFanin && Outputs[i]->idx > N->idx)) {
		    N = Outputs[i];
		    minFanin = Outputs[i]->Inputs.size();
		    ToCover = Outputs[i]->Uncovered;
		}
	    }
	}
	for(int i = 0; i < Internal.size(); i++) {
	    if(Internal[i]->Uncovered != ZERO) {
		if((Internal[i]->Inputs.size() < minFanin) ||
		   (Internal[i]->Inputs.size() == minFanin && Internal[i]->idx > N->idx)) {
		    N = Internal[i];
		    minFanin = Internal[i]->Inputs.size();
		    ToCover = Internal[i]->Uncovered;
		}
	    }
	}
	BDD M;
	if(ToCover.CountMinterm(INPUT) == 1) 
	    M = ToCover;
	else {
	    int minCovering = INF;
	    int minIndex = INF;
	    DdNode* inputnodes[INPUT];
	    for(int i = 0; i < INPUT; i++)
		inputnodes[i] = InputBdds[i].getNode();
	    BDDvector InputVector(INPUT, Manager, inputnodes);
	    while(ToCover != ZERO) {
		BDD minterm = ToCover.PickOneMinterm(InputVector);
		int ct = 0;
		for(int i = 0; i < N->Connectible.size(); i++)
		    if(minterm * !N->Connectible[i]->ON != ZERO) ct++;    
		if(ct < minCovering) { 
		    M = minterm;
		    minCovering = ct; 
		    minIndex = MintermIndex(M);
		} else if(ct == minCovering) { 
		    int tempIndex = MintermIndex(minterm);
		    if(tempIndex < minIndex) {
			M = minterm;
			minCovering = ct; 
			minIndex = MintermIndex(M);
		    }
		}
		ToCover *= !minterm;
	    }
	}
	Connectible.clear();
	for(int k = 0; k < N->Connectible.size(); k++) {
	    if((GateType == 1 && N->Connectible[k]->ON * M == ZERO) ||
	       (GateType == 2 && N->Connectible[k]->OFF * M == ZERO) ||
	       ((GateType == 3 || GateType == 4) && N->LogicOp == 'A' && N->Connectible[k]->ON * M == ZERO) ||
	       ((GateType == 3 || GateType == 4) && N->LogicOp == 'O' && N->Connectible[k]->OFF * M == ZERO) ||
	       (GateType == 3 && N->LogicOp == 'N' && N->Connectible[k]->ON * M == ZERO))
		Connectible.push_back(N->Connectible[k]);
	}
	return M;
    }
    //H: (1) Find the node with (a) smallest fan-in (b) smallest connectible set (c) largest index
    //   (2) Find the minterm with (a) fewest coverings possible (b) smallest index.
    if(MintermChoice == 'H') {
	N = NULL;
	BDD ToCover;
	if(print) { cout<<"Select Node: "<<endl<<"    node: fan-in, connectible"<<endl; }
	for(int i = Internal.size()-1; i >= 0; i--) {
	    if(Internal[i]->Uncovered != ZERO) {
		if(print) cout<<"    "<<Internal[i]<<": "<<Internal[i]->Inputs.size()<<", "<<Internal[i]->Connectible.size()<<endl;
		if(N == NULL || 
		   Internal[i]->Inputs.size() < N->Inputs.size() ||
		   (Internal[i]->Inputs.size() == N->Inputs.size() && Internal[i]->Connectible.size() < N->Connectible.size())) {
		    N = Internal[i];
		    ToCover = Internal[i]->Uncovered;
		}
	    }
	}
	for(int i = Outputs.size()-1; i >= 0; i--) {
	    if(Outputs[i]->Uncovered != ZERO) {
		if(print) cout<<"    "<<Outputs[i]<<": "<<Outputs[i]->Inputs.size()<<", "<<Outputs[i]->Connectible.size()<<endl;
		if(N == NULL || 
		   Outputs[i]->Inputs.size() < N->Inputs.size() ||
		   (Outputs[i]->Inputs.size() == N->Inputs.size() && Outputs[i]->Connectible.size() < N->Connectible.size())) {
		    N = Outputs[i];
		    ToCover = Outputs[i]->Uncovered;
		}
	    }
	}
	if(print) {
	    cout<<"  Selected Node = "<<N<<endl;
	    cout<<"  ToCover = "; PrintFunction(ToCover); cout<<endl;
	    cout<<"Select Minterm"<<endl;
	}
	BDD M;
	if(ToCover.CountMinterm(INPUT) == 1) 
	    M = ToCover;
	else {
	    //if(print) cout<<"  minterm, coverings, index"<<endl;
	    int minCovering = INF;
	    int minIndex; if(GateType == 1) minIndex = INF;  else if(GateType == 2) minIndex = -1;//V.1
	    //int minRank = INF;
	    DdNode* inputnodes[INPUT];
	    for(int i = 0; i < INPUT; i++)
		inputnodes[i] = InputBdds[i].getNode();
	    BDDvector InputVector(INPUT, Manager, inputnodes);
	    //BDD Minterms;  //V.2
	    while(ToCover != ZERO) {
		BDD minterm = ToCover.PickOneMinterm(InputVector);
		int ct = 0;
		for(int i = 0; i < N->Connectible.size(); i++)
		    if((GateType == 1 && minterm * !N->Connectible[i]->ON != ZERO) ||
		       (GateType == 2 && minterm * !N->Connectible[i]->OFF != ZERO) ||
		       ((GateType == 3 || GateType == 4) && N->LogicOp == 'A' && minterm * !N->Connectible[i]->ON != ZERO) ||
		       ((GateType == 3 || GateType == 4) && N->LogicOp == 'O' && minterm * !N->Connectible[i]->OFF != ZERO) ||
		       (GateType == 3 && N->LogicOp == 'N' && minterm * !N->Connectible[i]->ON != ZERO) ||
		       (GateType == 5 && N->LogicOp == 'D' && minterm * !N->Connectible[i]->ON != ZERO) ||
		       (GateType == 5 && N->LogicOp == 'R' && minterm * !N->Connectible[i]->OFF != ZERO))
		      ct++;
		//if(print) { cout<<"  "; PrintFunction(minterm); cout<<", "<<ct<<", "<<MintermIndex(minterm)<<endl; }
		if(ct < minCovering) { 
		    M = minterm;  //V.1
		    minCovering = ct; 
		    //minRank = MintermRank(M);
		    minIndex = MintermIndex(M);  //V.1
		    //Minterms = minterm; //V.2
		} else if(ct == minCovering) { 
		    //int tempRank = MintermRank(minterm);
		    //if(tempRank < minRank) {
		    //M = minterm;
		    //minCovering = ct;
		    //minRank = tempRank;
		    //minIndex = MintermIndex(M);
		    //} else if(tempRank == minRank) {
		    int tempIndex = MintermIndex(minterm); //V.1
		    if((GateType == 1 && tempIndex < minIndex) ||
		       (GateType == 2 && tempIndex > minIndex)) {  //V.1
			M = minterm;  //V.1
			minCovering = ct; 
			//minRank = tempRank;
			minIndex = tempIndex;  //V.1
			//Minterms += minterm; //V.2
		    }  //V.1
		    //}
		}
		ToCover *= !minterm;
	    }
	    //M = PickMinterm(Minterms); //V.2
	}
	if(print) { cout<<"  Selected Minterm = "; PrintFunction(M); cout<<endl; }
	Connectible.clear();
	for(int k = 0; k < N->Connectible.size(); k++) {
	    if((GateType == 1 && N->Connectible[k]->ON * M == ZERO) ||
	       (GateType == 2 && N->Connectible[k]->OFF * M == ZERO) ||
	       ((GateType == 3 || GateType == 4) && N->LogicOp == 'A' && N->Connectible[k]->ON * M == ZERO) ||
	       ((GateType == 3 || GateType == 4) && N->LogicOp == 'O' && N->Connectible[k]->OFF * M == ZERO) ||
	       (GateType == 3 && N->LogicOp == 'N' && N->Connectible[k]->ON * M == ZERO) ||
	       (GateType == 5 && N->LogicOp == 'D' && N->Connectible[k]->ON * M == ZERO) ||
	       (GateType == 5 && N->LogicOp == 'R' && N->Connectible[k]->OFF * M == ZERO))
	      Connectible.push_back(N->Connectible[k]);
	}
	return M;
    }
    //I-M need the following minterm/node labels:
    //  1.  FI = Can be covered by a Fan-In node
    //  2.  IN = Can be covered by an Input Node
    //  3.  GN = Can be covered by an existing Gate Node
    //  4.  NN = Can be covered by a New gate Node
    //I: (1) Find the set of minterms with the most difficult covering type (1, 2, 3, 4)
    //   (2) Randomly select a minterm from the set
    if(MintermChoice == 'I') {
	DdNode* inputnodes[INPUT];
	for(int i = 0; i < INPUT; i++)
	    inputnodes[i] = InputBdds[i].getNode();
	BDDvector InputVector(INPUT, Manager, inputnodes);
	vector<pair<Node*, BDD> > SavedMinterms;
	int maxLabel = 1;
	int total = 0;
	if(print) cout<<"Find Nodes with largest Label"<<endl<<"  node: label, mintermcount"<<endl;
	for(int i = Internal.size()-1; i >= 0; i--) {
	    if(Internal[i]->Label > 0 && print) 
		cout<<"  "<<Internal[i]<<": "<<Internal[i]->Label<<", "<<Internal[i]->UncoveredSet[Internal[i]->Label].CountMinterm(INPUT)<<endl;
	    if(Internal[i]->Label > maxLabel) {
		SavedMinterms.clear();
		total = 0;
		maxLabel = Internal[i]->Label;
	    }
	    if(Internal[i]->Label == maxLabel) {
		SavedMinterms.push_back(pair<Node*, BDD> (Internal[i], Internal[i]->UncoveredSet[maxLabel]));
		total += (int) Internal[i]->UncoveredSet[maxLabel].CountMinterm(INPUT);
	    }
	}
	for(int i = Outputs.size()-1; i >= 0; i--) {
	    if(Outputs[i]->Label > 0 && print) 
		cout<<"  "<<Outputs[i]<<": "<<Outputs[i]->Label<<", "<<Outputs[i]->UncoveredSet[Outputs[i]->Label].CountMinterm(INPUT)<<endl;
	    if(Outputs[i]->Label > maxLabel) {
		SavedMinterms.clear();
		total = 0;
		maxLabel = Outputs[i]->Label;
	    }
	    if(Outputs[i]->Label == maxLabel) {
		SavedMinterms.push_back(pair<Node*, BDD> (Outputs[i], Outputs[i]->UncoveredSet[maxLabel]));
		total += (int) Outputs[i]->UncoveredSet[maxLabel].CountMinterm(INPUT);
	    }
	}
	if(total == 1) {
	    BDD M = SavedMinterms[0].second;
	    N = SavedMinterms[0].first;
		if(print) { cout<<"  Node = "<<N<<endl<<"  Minterm = "; PrintFunction(M); cout<<endl; }
		//Get Connectible Set
		Connectible.clear();
		for(int k = 0; k < N->Connectible.size(); k++) {
		    if((GateType == 1 && N->Connectible[k]->ON * M == ZERO) ||
		       (GateType == 2 && N->Connectible[k]->OFF * M == ZERO) ||
		       ((GateType == 3 || GateType == 4) && N->LogicOp == 'A' && N->Connectible[k]->ON * M == ZERO) ||
		       ((GateType == 3 || GateType == 4) && N->LogicOp == 'O' && N->Connectible[k]->OFF * M == ZERO) ||
		       (GateType == 3 && N->LogicOp == 'N' && N->Connectible[k]->ON * M == ZERO) ||
		       (GateType == 5 && N->LogicOp == 'D' && N->Connectible[k]->ON * M == ZERO) ||
		       (GateType == 5 && N->LogicOp == 'R' && N->Connectible[k]->OFF * M == ZERO))
		      Connectible.push_back(N->Connectible[k]);
		}
		return M;
	} else {
	    //Randomly Select Minterm
	    double r = (double)rand() / ((double)(RAND_MAX)+(double)(1));
	    double x = r * total;
	    int index = (int)x;
	    if(print) {
		cout<<"Select Minterm "<<index<<" / "<<total<<endl;
		cout<<"  Minterm Options("<<maxLabel<<"): ";
		for(int i = 0; i < SavedMinterms.size(); i++) {
		    if(i != 0) cout<<", ";
		    cout<<"("<<SavedMinterms[i].first<<", "; PrintFunction(SavedMinterms[i].second); cout<<")";
		}
		cout<<endl;
	    }
	    int current = 0;
	    BDD M;
	    for(int i = 0; i < SavedMinterms.size(); i++) {
		current += (int) SavedMinterms[i].second.CountMinterm(INPUT);
		if(index < current) {
		    N = SavedMinterms[i].first;
		    for(int j = 0; j < current-index; j++) {
			M = SavedMinterms[i].second.PickOneMinterm(InputVector);
			SavedMinterms[i].second *= !M;
		    }
		    if(print) { cout<<"  Node = "<<N<<endl<<"  Minterm = "; PrintFunction(M); cout<<endl; }
		    //Get Connectible Set
		    Connectible.clear();
		    for(int k = 0; k < N->Connectible.size(); k++) {
			if((GateType == 1 && N->Connectible[k]->ON * M == ZERO) ||
			   (GateType == 2 && N->Connectible[k]->OFF * M == ZERO) ||
			   ((GateType == 3 || GateType == 4) && N->LogicOp == 'A' && N->Connectible[k]->ON * M == ZERO) ||
			   ((GateType == 3 || GateType == 4) && N->LogicOp == 'O' && N->Connectible[k]->OFF * M == ZERO) ||
			   (GateType == 3 && N->LogicOp == 'N' && N->Connectible[k]->ON * M == ZERO) ||
			   (GateType == 5 && N->LogicOp == 'D' && N->Connectible[k]->ON * M == ZERO) ||
			   (GateType == 5 && N->LogicOp == 'R' && N->Connectible[k]->OFF * M == ZERO))
			  Connectible.push_back(N->Connectible[k]);
		    }
		    return M;
		}
	    }
	}
    }
    //J: (1) Find the set of minterms with the most difficult covering type (1, 2, 3, 4)
    //   (2) Select the minterm with fewest covering options
    if(MintermChoice == 'J') {
	DdNode* inputnodes[INPUT];
	for(int i = 0; i < INPUT; i++)
	    inputnodes[i] = InputBdds[i].getNode();
	BDDvector InputVector(INPUT, Manager, inputnodes);
	vector<pair<Node*, BDD> > SavedMinterms;
	int maxLabel = 1;
	int total = 0;
	if(print) cout<<"Find Nodes with largest Label"<<endl<<"  node: label, mintermcount"<<endl;
	for(int i = Internal.size()-1; i >= 0; i--) {
	    if(Internal[i]->Label > 0 && print) 
		if(print) cout<<"  "<<Internal[i]<<": "<<Internal[i]->Label<<", "<<Internal[i]->UncoveredSet[Internal[i]->Label].CountMinterm(INPUT)<<endl;
	    if(Internal[i]->Label > maxLabel) {
		SavedMinterms.clear();
		total = 0;
		maxLabel = Internal[i]->Label;
	    }
	    if(Internal[i]->Label == maxLabel) {
		SavedMinterms.push_back(pair<Node*, BDD> (Internal[i], Internal[i]->UncoveredSet[maxLabel]));
		total += (int) Internal[i]->UncoveredSet[maxLabel].CountMinterm(INPUT);
	    }
	}
	for(int i = Outputs.size()-1; i >= 0; i--) {
	    if(Outputs[i]->Label > 0 && print) 
		if(print) cout<<"  "<<Outputs[i]<<": "<<Outputs[i]->Label<<", "<<Outputs[i]->UncoveredSet[Outputs[i]->Label].CountMinterm(INPUT)<<endl;
	    if(Outputs[i]->Label > maxLabel) {
		SavedMinterms.clear();
		total = 0;
		maxLabel = Outputs[i]->Label;
	    }
	    if(Outputs[i]->Label == maxLabel) {
		SavedMinterms.push_back(pair<Node*, BDD> (Outputs[i], Outputs[i]->UncoveredSet[maxLabel]));
		total += (int) Outputs[i]->UncoveredSet[maxLabel].CountMinterm(INPUT);
	    }
	}
	BDD M;
	if(total == 1) {
	    M = SavedMinterms[0].second;
	    N = SavedMinterms[0].first;
	} else {
	    //Find the minterm with fewest covering options
	    int CoveringOptions = INF;
	    if(print) cout<<"  minterm, node, coverings"<<endl;
	    for(int i = 0; i < SavedMinterms.size(); i++) {
		Node* tempN = SavedMinterms[i].first;
		while(SavedMinterms[i].second != ZERO) {
		    BDD tempMinterm = SavedMinterms[i].second.PickOneMinterm(InputVector);
		    SavedMinterms[i].second *= !tempMinterm;
		    int ct = 0;
		    for(int i = 0; i < tempN->Connectible.size(); i++)
			if((GateType == 1 && tempMinterm * !tempN->Connectible[i]->ON != ZERO) ||
			   (GateType == 2 && tempMinterm * !tempN->Connectible[i]->OFF != ZERO) ||
			   ((GateType == 3 || GateType == 4) && tempN->LogicOp == 'A' && !tempN->Connectible[i]->ON * tempMinterm != ZERO) ||
			   ((GateType == 3 || GateType == 4) && tempN->LogicOp == 'O' && !tempN->Connectible[i]->OFF * tempMinterm != ZERO) ||
			   (GateType == 3 && tempN->LogicOp == 'N' && !tempN->Connectible[i]->ON * tempMinterm != ZERO) ||
			   (GateType == 5 && tempN->LogicOp == 'D' && tempMinterm * !tempN->Connectible[i]->ON != ZERO) ||
			   (GateType == 5 && tempN->LogicOp == 'R' && tempMinterm * !tempN->Connectible[i]->OFF != ZERO))
			  ct++;
		    if(print) { cout<<"  "; PrintFunction(tempMinterm); cout<<", "<<tempN<<", "<<ct<<endl; }
		    if(ct < CoveringOptions) { 
			M = tempMinterm;
			N = tempN;
			CoveringOptions = ct;
		    }
		}
	    }
	}
	if(print) { cout<<"  Node = "<<N<<endl<<"  Minterm = "; PrintFunction(M); cout<<endl; }
	//Get Connectible Set
	Connectible.clear();
	for(int k = 0; k < N->Connectible.size(); k++) {
	    if((GateType == 1 && N->Connectible[k]->ON * M == ZERO) ||
	       (GateType == 2 && N->Connectible[k]->OFF * M == ZERO) ||
	       ((GateType == 3 || GateType == 4) && N->LogicOp == 'A' && N->Connectible[k]->ON * M == ZERO) ||
	       ((GateType == 3 || GateType == 4) && N->LogicOp == 'O' && N->Connectible[k]->OFF * M == ZERO) ||
	       (GateType == 3 && N->LogicOp == 'N' && N->Connectible[k]->ON * M == ZERO) ||
	       (GateType == 5 && N->LogicOp == 'D' && N->Connectible[k]->ON * M == ZERO) ||
	       (GateType == 5 && N->LogicOp == 'R' && N->Connectible[k]->OFF * M == ZERO))
	      Connectible.push_back(N->Connectible[k]);
	}
	return M;
    }


    //K: (1) Find the set of minterms with the most difficult covering type (2, 1, 3, 4)
    //   (2) Randomly select a minterm from the set
    if(MintermChoice == 'K') {
	DdNode* inputnodes[INPUT];
	for(int i = 0; i < INPUT; i++)
	    inputnodes[i] = InputBdds[i].getNode();
	BDDvector InputVector(INPUT, Manager, inputnodes);
	vector<pair<Node*, BDD> > SavedMinterms;
	int maxLabel = 1;
	int total = 0;
	if(print) cout<<"Find Nodes with largest Label"<<endl<<"  node: label, mintermcount"<<endl;
	for(int i = Internal.size()-1; i >= 0; i--) {
	    if(Internal[i]->Label > 0 && print) 
		cout<<"  "<<Internal[i]<<": "<<Internal[i]->Label<<", "<<Internal[i]->UncoveredSet[Internal[i]->Label].CountMinterm(INPUT)<<endl;
	    if(Internal[i]->Label > maxLabel) {
		SavedMinterms.clear();
		total = 0;
		maxLabel = Internal[i]->Label;
	    }
	    if(Internal[i]->Label == maxLabel) {
		SavedMinterms.push_back(pair<Node*, BDD> (Internal[i], Internal[i]->UncoveredSet[maxLabel]));
		total += (int) Internal[i]->UncoveredSet[maxLabel].CountMinterm(INPUT);
	    }
	}
	for(int i = Outputs.size()-1; i >= 0; i--) {
	    if(Outputs[i]->Label > 0 && print) 
		cout<<"  "<<Outputs[i]<<": "<<Outputs[i]->Label<<", "<<Outputs[i]->UncoveredSet[Outputs[i]->Label].CountMinterm(INPUT)<<endl;
	    if(Outputs[i]->Label > maxLabel) {
		SavedMinterms.clear();
		total = 0;
		maxLabel = Outputs[i]->Label;
	    }
	    if(Outputs[i]->Label == maxLabel) {
		SavedMinterms.push_back(pair<Node*, BDD> (Outputs[i], Outputs[i]->UncoveredSet[maxLabel]));
		total += (int) Outputs[i]->UncoveredSet[maxLabel].CountMinterm(INPUT);
	    }
	}
	if(total == 1) {
	    BDD M = SavedMinterms[0].second;
	    N = SavedMinterms[0].first;
		if(print) { cout<<"  Node = "<<N<<endl<<"  Minterm = "; PrintFunction(M); cout<<endl; }
		//Get Connectible Set
		Connectible.clear();
		for(int k = 0; k < N->Connectible.size(); k++) {
		    if((GateType == 1 && N->Connectible[k]->ON * M == ZERO) ||
		       (GateType == 2 && N->Connectible[k]->OFF * M == ZERO) ||
		       ((GateType == 3 || GateType == 4) && N->LogicOp == 'A' && N->Connectible[k]->ON * M == ZERO) ||
		       ((GateType == 3 || GateType == 4) && N->LogicOp == 'O' && N->Connectible[k]->OFF * M == ZERO) ||
		       (GateType == 3 && N->LogicOp == 'N' && N->Connectible[k]->ON * M == ZERO) ||
		       (GateType == 5 && N->LogicOp == 'D' && N->Connectible[k]->ON * M == ZERO) ||
		       (GateType == 5 && N->LogicOp == 'R' && N->Connectible[k]->OFF * M == ZERO))
		      Connectible.push_back(N->Connectible[k]);
		}
		return M;
	} else {
	    //Randomly Select Minterm
	    double r = (double)rand() / ((double)(RAND_MAX)+(double)(1));
	    double x = r * total;
	    int index = (int)x;
	    if(print) {
		cout<<"Select Minterm "<<index<<" / "<<total<<endl;
		cout<<"  Minterm Options("<<maxLabel<<"): ";
		for(int i = 0; i < SavedMinterms.size(); i++) {
		    if(i != 0) cout<<", ";
		    cout<<"("<<SavedMinterms[i].first<<", "; PrintFunction(SavedMinterms[i].second); cout<<")";
		}
		cout<<endl;
	    }
	    int current = 0;
	    BDD M;
	    for(int i = 0; i < SavedMinterms.size(); i++) {
		current += (int) SavedMinterms[i].second.CountMinterm(INPUT);
		if(index < current) {
		    N = SavedMinterms[i].first;
		    for(int j = 0; j < current-index; j++) {
			M = SavedMinterms[i].second.PickOneMinterm(InputVector);
			SavedMinterms[i].second *= !M;
		    }
		    if(print) { cout<<"  Node = "<<N<<endl<<"  Minterm = "; PrintFunction(M); cout<<endl; }
		    //Get Connectible Set
		    Connectible.clear();
		    for(int k = 0; k < N->Connectible.size(); k++) {
			if((GateType == 1 && N->Connectible[k]->ON * M == ZERO) ||
			   (GateType == 2 && N->Connectible[k]->OFF * M == ZERO) ||
			   ((GateType == 3 || GateType == 4) && N->LogicOp == 'A' && N->Connectible[k]->ON * M == ZERO) ||
			   ((GateType == 3 || GateType == 4) && N->LogicOp == 'O' && N->Connectible[k]->OFF * M == ZERO) ||
			   (GateType == 3 && N->LogicOp == 'N' && N->Connectible[k]->ON * M == ZERO))
			    Connectible.push_back(N->Connectible[k]);
		    }
		    return M;
		}
	    }
	}
    }
    //L: (1) Find the set of minterms with the most difficult covering type (2, 1, 3, 4)
    //   (2) Select the minterm with fewest covering options
    if(MintermChoice == 'L') {
	DdNode* inputnodes[INPUT];
	for(int i = 0; i < INPUT; i++)
	    inputnodes[i] = InputBdds[i].getNode();
	BDDvector InputVector(INPUT, Manager, inputnodes);
	vector<pair<Node*, BDD> > SavedMinterms;
	int maxLabel = 1;
	int total = 0;
	if(print) cout<<"Find Nodes with largest Label"<<endl<<"  node: label, mintermcount"<<endl;
	for(int i = Internal.size()-1; i >= 0; i--) {
	    if(Internal[i]->Label > 0 && print) 
		cout<<"  "<<Internal[i]<<": "<<Internal[i]->Label<<", "<<Internal[i]->UncoveredSet[Internal[i]->Label].CountMinterm(INPUT)<<endl;
	    if(Internal[i]->Label > maxLabel) {
		SavedMinterms.clear();
		total = 0;
		maxLabel = Internal[i]->Label;
	    }
	    if(Internal[i]->Label == maxLabel) {
		SavedMinterms.push_back(pair<Node*, BDD> (Internal[i], Internal[i]->UncoveredSet[maxLabel]));
		total += (int) Internal[i]->UncoveredSet[maxLabel].CountMinterm(INPUT);
	    }
	}
	for(int i = Outputs.size()-1; i >= 0; i--) {
	    if(Outputs[i]->Label > 0 && print) 
		cout<<"  "<<Outputs[i]<<": "<<Outputs[i]->Label<<", "<<Outputs[i]->UncoveredSet[Outputs[i]->Label].CountMinterm(INPUT)<<endl;
	    if(Outputs[i]->Label > maxLabel) {
		SavedMinterms.clear();
		total = 0;
		maxLabel = Outputs[i]->Label;
	    }
	    if(Outputs[i]->Label == maxLabel) {
		SavedMinterms.push_back(pair<Node*, BDD> (Outputs[i], Outputs[i]->UncoveredSet[maxLabel]));
		total += (int) Outputs[i]->UncoveredSet[maxLabel].CountMinterm(INPUT);
	    }
	}
	BDD M;
	if(total == 1) {
	    M = SavedMinterms[0].second;
	    N = SavedMinterms[0].first;
	} else {
	    //Find the minterm with fewest covering options
	    int CoveringOptions = INF;
	    if(print) cout<<"  minterm, node, coverings"<<endl;
	    for(int i = 0; i < SavedMinterms.size(); i++) {
		Node* tempN = SavedMinterms[i].first;
		while(SavedMinterms[i].second != ZERO) {
		    BDD tempMinterm = SavedMinterms[i].second.PickOneMinterm(InputVector);
		    SavedMinterms[i].second *= !tempMinterm;
		    int ct = 0;
		    for(int i = 0; i < tempN->Connectible.size(); i++)
			if((GateType == 1 && tempMinterm * !tempN->Connectible[i]->ON != ZERO) ||
			   (GateType == 2 && tempMinterm * !tempN->Connectible[i]->OFF != ZERO) ||
			   ((GateType == 3 || GateType == 4) && tempN->LogicOp == 'A' && !tempN->Connectible[i]->ON * tempMinterm != ZERO) ||
			   ((GateType == 3 || GateType == 4) && tempN->LogicOp == 'O' && !tempN->Connectible[i]->OFF * tempMinterm != ZERO) ||
			   (GateType == 3 && tempN->LogicOp == 'N' && !tempN->Connectible[i]->ON * tempMinterm != ZERO))
			    ct++;
		    if(print) { cout<<"  "; PrintFunction(tempMinterm); cout<<", "<<tempN<<", "<<ct<<endl; }
		    if(ct < CoveringOptions) { 
			M = tempMinterm;
			N = tempN;
			CoveringOptions = ct;
		    }
		}
	    }
	}
	if(print) { cout<<"  Node = "<<N<<endl<<"  Minterm = "; PrintFunction(M); cout<<endl; }
	//Get Connectible Set
	Connectible.clear();
	for(int k = 0; k < N->Connectible.size(); k++) {
	    if((GateType == 1 && N->Connectible[k]->ON * M == ZERO) ||
	       (GateType == 2 && N->Connectible[k]->OFF * M == ZERO) ||
	       ((GateType == 3 || GateType == 4) && N->LogicOp == 'A' && N->Connectible[k]->ON * M == ZERO) ||
	       ((GateType == 3 || GateType == 4) && N->LogicOp == 'O' && N->Connectible[k]->OFF * M == ZERO) ||
	       (GateType == 3 && N->LogicOp == 'N' && N->Connectible[k]->ON * M == ZERO))
		Connectible.push_back(N->Connectible[k]);
	}
	return M;
    }
    //M: (1) Find the set of minterms with the most difficult covering type (1, 2, 3, 4)
    //   (2) Reduce the set of minterms to those with fewest covering options
    //   (3) Differentiate between type 'GN' minterms as follows
    //       (1) No on/off-set change needed, (2) only off-set changes, (3) only on-set changes, (4) both on & off-set changes
    if(MintermChoice == 'M') {
	DdNode* inputnodes[INPUT];
	for(int i = 0; i < INPUT; i++)
	    inputnodes[i] = InputBdds[i].getNode();
	BDDvector InputVector(INPUT, Manager, inputnodes);
	vector<pair<Node*, BDD> > SavedMinterms;
	int maxLabel = 1;
	int total = 0;
	if(print) cout<<"Find Nodes with largest Label"<<endl<<"  node: label, mintermcount"<<endl;
	for(int i = Internal.size()-1; i >= 0; i--) {
	    if(Internal[i]->Label > 0 && print) 
		if(print) cout<<"  "<<Internal[i]<<": "<<Internal[i]->Label<<", "<<Internal[i]->UncoveredSet[Internal[i]->Label].CountMinterm(INPUT)<<endl;
	    if(Internal[i]->Label > maxLabel) {
		SavedMinterms.clear();
		total = 0;
		maxLabel = Internal[i]->Label;
	    }
	    if(Internal[i]->Label == maxLabel) {
		SavedMinterms.push_back(pair<Node*, BDD> (Internal[i], Internal[i]->UncoveredSet[maxLabel]));
		total += (int) Internal[i]->UncoveredSet[maxLabel].CountMinterm(INPUT);
	    }
	}
	for(int i = Outputs.size()-1; i >= 0; i--) {
	    if(Outputs[i]->Label > 0 && print) 
		if(print) cout<<"  "<<Outputs[i]<<": "<<Outputs[i]->Label<<", "<<Outputs[i]->UncoveredSet[Outputs[i]->Label].CountMinterm(INPUT)<<endl;
	    if(Outputs[i]->Label > maxLabel) {
		SavedMinterms.clear();
		total = 0;
		maxLabel = Outputs[i]->Label;
	    }
	    if(Outputs[i]->Label == maxLabel) {
		SavedMinterms.push_back(pair<Node*, BDD> (Outputs[i], Outputs[i]->UncoveredSet[maxLabel]));
		total += (int) Outputs[i]->UncoveredSet[maxLabel].CountMinterm(INPUT);
	    }
	}
	BDD M;
	if(total == 1) {
	    M = SavedMinterms[0].second;
	    N = SavedMinterms[0].first;
	} else {
	    //Find the minterms with fewest covering options
	    int CoveringOptions = INF;
	    vector<BDD> Minterms;
	    vector<Node*> Nodes;
	    if(print) cout<<"  minterm, node, coverings"<<endl;
	    for(int i = 0; i < SavedMinterms.size(); i++) {
		Node* tempN = SavedMinterms[i].first;
		while(SavedMinterms[i].second != ZERO) {
		    BDD tempMinterm = SavedMinterms[i].second.PickOneMinterm(InputVector);
		    SavedMinterms[i].second *= !tempMinterm;
		    int ct = 0;
		    for(int i = 0; i < tempN->Connectible.size(); i++)
			if((GateType == 1 && tempMinterm * !tempN->Connectible[i]->ON != ZERO) ||
			   (GateType == 2 && tempMinterm * !tempN->Connectible[i]->OFF != ZERO) ||
			   ((GateType == 3 || GateType == 4) && tempN->LogicOp == 'A' && !tempN->Connectible[i]->ON * tempMinterm != ZERO) ||
			   ((GateType == 3 || GateType == 4) && tempN->LogicOp == 'O' && !tempN->Connectible[i]->OFF * tempMinterm != ZERO) ||
			   (GateType == 3 && tempN->LogicOp == 'N' && !tempN->Connectible[i]->ON * tempMinterm != ZERO))
			    ct++;
		    if(print) { cout<<"  "; PrintFunction(tempMinterm); cout<<", "<<tempN<<", "<<ct<<endl; }
		    if(ct < CoveringOptions) {
			Minterms.clear();
			Nodes.clear(); 
			CoveringOptions = ct;
		    }
		    if(ct == CoveringOptions) {
			Minterms.push_back(tempMinterm);
			Nodes.push_back(tempN);
		    }
		}
	    }
	    //Differentiate between type 'GN' minterms as follows
	    // (1) No on/off-set change needed, (2) only off-set changes, (3) only on-set changes, (4) both on & off-set changes
	    if(maxLabel == 3 && Minterms.size() > 1) {
		int MaxGNLabel = 0;
		if(print) cout<<"Differentiate GN type"<<endl<<"  minterm, node, GNtype"<<endl;
		for(int i = 0; i < Minterms.size(); i++) {
		    int GNLabel = 5;
		    for(int j = 0; j < Nodes[i]->Connectible.size(); j++) {
			if(!Nodes[i]->Connectible[j]->IsInput()) {
			    if((GateType == 1 && Nodes[i]->Connectible[j]->ON * Minterms[i] == ZERO) ||
			       (GateType == 2 && Nodes[i]->Connectible[j]->OFF * Minterms[i] == ZERO) ||
			       ((GateType == 3 || GateType == 4) && Nodes[i]->LogicOp == 'A' && Nodes[i]->Connectible[j]->ON * Minterms[i] == ZERO) ||
			       ((GateType == 3 || GateType == 4) && Nodes[i]->LogicOp == 'O' && Nodes[i]->Connectible[j]->OFF * Minterms[i] == ZERO) ||
			       (GateType == 3 && Nodes[i]->LogicOp == 'N' && Nodes[i]->Connectible[j]->ON * Minterms[i] == ZERO)) {
				int jLabel = 1;
				//Off-set Changes
				if((GateType == 1 && Nodes[i]->Connectible[j]->OFF * Minterms[i] == ZERO) ||
				   (GateType == 2 && Nodes[i]->Connectible[j]->ON * Minterms[i] == ZERO) ||
				   ((GateType == 3 || GateType == 4) && Nodes[i]->LogicOp == 'A' && Nodes[i]->Connectible[j]->OFF * Minterms[i] == ZERO) ||
				   ((GateType == 3 || GateType == 4) && Nodes[i]->LogicOp == 'O' && Nodes[i]->Connectible[j]->ON * Minterms[i] == ZERO) ||
				   (GateType == 3 && Nodes[i]->LogicOp == 'N' && Nodes[i]->Connectible[j]->OFF * Minterms[i] == ZERO))
				    jLabel = 2;
				//On-set Changes
				if((GateType == 1 && Nodes[i]->Connectible[j]->ON * Nodes[i]->OFF != Nodes[i]->OFF) ||
				   (GateType == 2 && Nodes[i]->Connectible[j]->OFF * Nodes[i]->ON != Nodes[i]->ON) ||
				   ((GateType == 3 || GateType == 4) && Nodes[i]->LogicOp == 'A' && Nodes[i]->Connectible[j]->ON * Nodes[i]->ON != Nodes[i]->ON) ||
				   ((GateType == 3 || GateType == 4) && Nodes[i]->LogicOp == 'O' && Nodes[i]->Connectible[j]->OFF * Nodes[i]->OFF != Nodes[i]->OFF) ||
				   (GateType == 3 && Nodes[i]->LogicOp == 'N' && Nodes[i]->Connectible[j]->ON * Nodes[i]->OFF != Nodes[i]->OFF))
				    jLabel += 2;
				if(jLabel < GNLabel) GNLabel = jLabel;
			    }
			}
		    }
		    if(print) { cout<<"  "; PrintFunction(Minterms[i]); cout<<", "<<Nodes[i]<<", "<<GNLabel<<endl; }
		    if(GNLabel > MaxGNLabel) {
			MaxGNLabel = GNLabel;
			M = Minterms[i];
			N = Nodes[i];
		    }
		}
	    } else {
		N = Nodes[0];
		M = Minterms[0];
	    }
	}
	if(print) { cout<<"  Node = "<<N<<endl<<"  Minterm = "; PrintFunction(M); cout<<endl; }
	//Get Connectible Set
	Connectible.clear();
	for(int k = 0; k < N->Connectible.size(); k++) {
	    if((GateType == 1 && N->Connectible[k]->ON * M == ZERO) ||
	       (GateType == 2 && N->Connectible[k]->OFF * M == ZERO) ||
	       ((GateType == 3 || GateType == 4) && N->LogicOp == 'A' && N->Connectible[k]->ON * M == ZERO) ||
	       ((GateType == 3 || GateType == 4) && N->LogicOp == 'O' && N->Connectible[k]->OFF * M == ZERO) ||
	       (GateType == 3 && N->LogicOp == 'N' && N->Connectible[k]->ON * M == ZERO))
		Connectible.push_back(N->Connectible[k]);
	}
	return M;
    }

    //N: (1) Find the set of minterms with the most difficult covering type (2, 1, 3, 4)
    //   (2) Reduce the set of minterms to those with fewest covering options
    //   (3) Differentiate between type 'GN' minterms as follows
    if(MintermChoice == 'N') {
	DdNode* inputnodes[INPUT];
	for(int i = 0; i < INPUT; i++)
	    inputnodes[i] = InputBdds[i].getNode();
	BDDvector InputVector(INPUT, Manager, inputnodes);
	vector<pair<Node*, BDD> > SavedMinterms;
	int maxLabel = 1;
	int total = 0;
	if(print) cout<<"Find Nodes with largest Label"<<endl<<"  node: label, mintermcount"<<endl;
	for(int i = Internal.size()-1; i >= 0; i--) {
	    if(Internal[i]->Label > 0 && print) 
		if(print) cout<<"  "<<Internal[i]<<": "<<Internal[i]->Label<<", "<<Internal[i]->UncoveredSet[Internal[i]->Label].CountMinterm(INPUT)<<endl;
	    if(Internal[i]->Label > maxLabel) {
		SavedMinterms.clear();
		total = 0;
		maxLabel = Internal[i]->Label;
	    }
	    if(Internal[i]->Label == maxLabel) {
		SavedMinterms.push_back(pair<Node*, BDD> (Internal[i], Internal[i]->UncoveredSet[maxLabel]));
		total += (int) Internal[i]->UncoveredSet[maxLabel].CountMinterm(INPUT);
	    }
	}
	for(int i = Outputs.size()-1; i >= 0; i--) {
	    if(Outputs[i]->Label > 0 && print) 
		if(print) cout<<"  "<<Outputs[i]<<": "<<Outputs[i]->Label<<", "<<Outputs[i]->UncoveredSet[Outputs[i]->Label].CountMinterm(INPUT)<<endl;
	    if(Outputs[i]->Label > maxLabel) {
		SavedMinterms.clear();
		total = 0;
		maxLabel = Outputs[i]->Label;
	    }
	    if(Outputs[i]->Label == maxLabel) {
		SavedMinterms.push_back(pair<Node*, BDD> (Outputs[i], Outputs[i]->UncoveredSet[maxLabel]));
		total += (int) Outputs[i]->UncoveredSet[maxLabel].CountMinterm(INPUT);
	    }
	}
	BDD M;
	if(total == 1) {
	    M = SavedMinterms[0].second;
	    N = SavedMinterms[0].first;
	} else {
	    //Find the minterms with fewest covering options
	    int CoveringOptions = INF;
	    vector<BDD> Minterms;
	    vector<Node*> Nodes;
	    if(print) cout<<"  minterm, node, coverings"<<endl;
	    for(int i = 0; i < SavedMinterms.size(); i++) {
		Node* tempN = SavedMinterms[i].first;
		while(SavedMinterms[i].second != ZERO) {
		    BDD tempMinterm = SavedMinterms[i].second.PickOneMinterm(InputVector);
		    SavedMinterms[i].second *= !tempMinterm;
		    int ct = 0;
		    for(int i = 0; i < tempN->Connectible.size(); i++)
			if((GateType == 1 && tempMinterm * !tempN->Connectible[i]->ON != ZERO) ||
			   (GateType == 2 && tempMinterm * !tempN->Connectible[i]->OFF != ZERO) ||
			   ((GateType == 3 || GateType == 4) && tempN->LogicOp == 'A' && !tempN->Connectible[i]->ON * tempMinterm != ZERO) ||
			   ((GateType == 3 || GateType == 4) && tempN->LogicOp == 'O' && !tempN->Connectible[i]->OFF * tempMinterm != ZERO) ||
			   (GateType == 3 && tempN->LogicOp == 'N' && !tempN->Connectible[i]->ON * tempMinterm != ZERO))
			    ct++;
		    if(print) { cout<<"  "; PrintFunction(tempMinterm); cout<<", "<<tempN<<", "<<ct<<endl; }
		    if(ct < CoveringOptions) {
			Minterms.clear();
			Nodes.clear(); 
			CoveringOptions = ct;
		    }
		    if(ct == CoveringOptions) {
			Minterms.push_back(tempMinterm);
			Nodes.push_back(tempN);
		    }
		}
	    }
	    //Differentiate between type 'GN' minterms as follows
	    // (1) No on/off-set change needed, (2) only off-set changes, (3) only on-set changes, (4) both on & off-set changes
	    if(maxLabel == 3 && Minterms.size() > 1) {
		int MaxGNLabel = 0;
		if(print) cout<<"Differentiate GN type"<<endl<<"  minterm, node, GNtype"<<endl;
		for(int i = 0; i < Minterms.size(); i++) {
		    int GNLabel = 5;
		    for(int j = 0; j < Nodes[i]->Connectible.size(); j++) {
			if(!Nodes[i]->Connectible[j]->IsInput()) {
			    if((GateType == 1 && Nodes[i]->Connectible[j]->ON * Minterms[i] == ZERO) ||
			       (GateType == 2 && Nodes[i]->Connectible[j]->OFF * Minterms[i] == ZERO) ||
			       ((GateType == 3 || GateType == 4) && Nodes[i]->LogicOp == 'A' && Nodes[i]->Connectible[j]->ON * Minterms[i] == ZERO) ||
			       ((GateType == 3 || GateType == 4) && Nodes[i]->LogicOp == 'O' && Nodes[i]->Connectible[j]->OFF * Minterms[i] == ZERO) ||
			       (GateType == 3 && Nodes[i]->LogicOp == 'N' && Nodes[i]->Connectible[j]->ON * Minterms[i] == ZERO)) {
				int jLabel = 1;
				//Off-set Changes
				if((GateType == 1 && Nodes[i]->Connectible[j]->OFF * Minterms[i] == ZERO) ||
				   (GateType == 2 && Nodes[i]->Connectible[j]->ON * Minterms[i] == ZERO) ||
				   ((GateType == 3 || GateType == 4) && Nodes[i]->LogicOp == 'A' && Nodes[i]->Connectible[j]->OFF * Minterms[i] == ZERO) ||
				   ((GateType == 3 || GateType == 4) && Nodes[i]->LogicOp == 'O' && Nodes[i]->Connectible[j]->ON * Minterms[i] == ZERO) ||
				   (GateType == 3 && Nodes[i]->LogicOp == 'N' && Nodes[i]->Connectible[j]->OFF * Minterms[i] == ZERO))
				    jLabel = 2;
				//On-set Changes
				if((GateType == 1 && Nodes[i]->Connectible[j]->ON * Nodes[i]->OFF != Nodes[i]->OFF) ||
				   (GateType == 2 && Nodes[i]->Connectible[j]->OFF * Nodes[i]->ON != Nodes[i]->ON) ||
				   ((GateType == 3 || GateType == 4) && Nodes[i]->LogicOp == 'A' && Nodes[i]->Connectible[j]->ON * Nodes[i]->ON != Nodes[i]->ON) ||
				   ((GateType == 3 || GateType == 4) && Nodes[i]->LogicOp == 'O' && Nodes[i]->Connectible[j]->OFF * Nodes[i]->OFF != Nodes[i]->OFF) ||
				   (GateType == 3 && Nodes[i]->LogicOp == 'N' && Nodes[i]->Connectible[j]->ON * Nodes[i]->OFF != Nodes[i]->OFF))
				    jLabel += 2;
				if(jLabel < GNLabel) GNLabel = jLabel;
			    }
			}
		    }
		    if(print) { cout<<"  "; PrintFunction(Minterms[i]); cout<<", "<<Nodes[i]<<", "<<GNLabel<<endl; }
		    if(GNLabel > MaxGNLabel) {
			MaxGNLabel = GNLabel;
			M = Minterms[i];
			N = Nodes[i];
		    }
		}
	    } else {
		N = Nodes[0];
		M = Minterms[0];
	    }
	}
	if(print) { cout<<"  Node = "<<N<<endl<<"  Minterm = "; PrintFunction(M); cout<<endl; }
	//Get Connectible Set
	Connectible.clear();
	for(int k = 0; k < N->Connectible.size(); k++) {
	    if((GateType == 1 && N->Connectible[k]->ON * M == ZERO) ||
	       (GateType == 2 && N->Connectible[k]->OFF * M == ZERO) ||
	       ((GateType == 3 || GateType == 4) && N->LogicOp == 'A' && N->Connectible[k]->ON * M == ZERO) ||
	       ((GateType == 3 || GateType == 4) && N->LogicOp == 'O' && N->Connectible[k]->OFF * M == ZERO) ||
	       (GateType == 3 && N->LogicOp == 'N' && N->Connectible[k]->ON * M == ZERO))
		Connectible.push_back(N->Connectible[k]);
	}
	return M;
    }
	
    //O: (1) Select the node with 
    //       (a) the most difficult covering type (IN, FI, GN, NN), 
    //       (b) smallest fan-in, 
    //       (c) smallest connectible set, 
    //       (d) largest index
    //   (2) Select the minterm with 
    //       (a) most difficult covering type (IN, FI, GN, NN), 
    //       (b) fewest covering options, 
    //       (c) smallest index
    if(MintermChoice == 'O') {
	int maxLabel = 1;
	int minFanin = INF;
	int minConn = INF;
	BDD ToCover;
	if(print) cout<<"Select Node with largest Label, smallest fan-in, smallest connectible, largest index"<<endl<<"  node: label, fan-in, conn"<<endl;
	for(int i = Internal.size()-1; i >= 0; i--) {
	    if(Internal[i]->Label > 0 && print) 
		cout<<"  "<<Internal[i]<<": "<<Internal[i]->Label<<", "<<Internal[i]->Inputs.size()<<", "<<Internal[i]->Connectible.size()<<endl;
	    if(Internal[i]->Label > maxLabel ||
	       (Internal[i]->Label == maxLabel && Internal[i]->Inputs.size() < minFanin) ||
	       (Internal[i]->Label == maxLabel && Internal[i]->Inputs.size() == minFanin && Internal[i]->Connectible.size() < minConn)) {
		maxLabel = Internal[i]->Label;
		minFanin = Internal[i]->Inputs.size();
		minConn = Internal[i]->Connectible.size();
		N = Internal[i];
		ToCover = Internal[i]->UncoveredSet[maxLabel];
	    }
	}
	for(int i = Outputs.size()-1; i >= 0; i--) {
	    if(Outputs[i]->Label > 0 && print) 
		cout<<"  "<<Outputs[i]<<": "<<Outputs[i]->Label<<", "<<Outputs[i]->Inputs.size()<<", "<<Outputs[i]->Connectible.size()<<endl;
	    if(Outputs[i]->Label > maxLabel ||
	       (Outputs[i]->Label == maxLabel && Outputs[i]->Inputs.size() < minFanin) ||
	       (Outputs[i]->Label == maxLabel && Outputs[i]->Inputs.size() == minFanin && Outputs[i]->Connectible.size() < minConn)) {
		maxLabel = Outputs[i]->Label;
		minFanin = Outputs[i]->Inputs.size();
		minConn = Outputs[i]->Connectible.size();
		N = Outputs[i];
		ToCover = Outputs[i]->UncoveredSet[maxLabel];
	    }
	}
	if(print) {
	    cout<<"  Selected Node = "<<N<<endl;
	    cout<<"  ToCover = "; PrintFunction(ToCover); cout<<endl;
	    cout<<"Select Minterm with largest Label, smallest covering, smallest index"<<endl;
	}
	BDD M;
	if(ToCover.CountMinterm(INPUT) == 1) 
	    M = ToCover;
	else {
	    if(print) cout<<"  minterm: label, coverings, index"<<endl;
	    int minCovering = INF;
	    int minIndex = INF;
	    DdNode* inputnodes[INPUT];
	    for(int i = 0; i < INPUT; i++)
		inputnodes[i] = InputBdds[i].getNode();
	    BDDvector InputVector(INPUT, Manager, inputnodes);
	    while(ToCover != ZERO) {
		BDD minterm = ToCover.PickOneMinterm(InputVector);
		int ct = 0;
		for(int i = 0; i < N->Connectible.size(); i++)
		    if((GateType == 1 && minterm * !N->Connectible[i]->ON != ZERO) ||
		       (GateType == 2 && minterm * !N->Connectible[i]->OFF != ZERO) ||
		       ((GateType == 3 || GateType == 4) && N->LogicOp == 'A' && !N->Connectible[i]->ON * minterm != ZERO) ||
		       ((GateType == 3 || GateType == 4) && N->LogicOp == 'O' && !N->Connectible[i]->OFF * minterm != ZERO) ||
		       (GateType == 3 && N->LogicOp == 'N' && !N->Connectible[i]->ON * minterm != ZERO))
			ct++;
		if(print) { cout<<"  "; PrintFunction(minterm); cout<<", "<<maxLabel<<", "<<ct<<", "<<MintermIndex(minterm)<<endl; }
		if(ct < minCovering) { 
		    M = minterm;
		    minCovering = ct; 
		    minIndex = MintermIndex(M);
		} else if(ct == minCovering) { 
		    int tempIndex = MintermIndex(minterm);
		    if(tempIndex < minIndex) {
			M = minterm;
			minCovering = ct; 
			minIndex = tempIndex;
		    }
		}
		ToCover *= !minterm;
	    }
	}
	if(print) { cout<<"  Selected Minterm = "; PrintFunction(M); cout<<endl; }
	Connectible.clear();
	for(int k = 0; k < N->Connectible.size(); k++) {
	    if((GateType == 1 && N->Connectible[k]->ON * M == ZERO) ||
	       (GateType == 2 && N->Connectible[k]->OFF * M == ZERO) ||
	       ((GateType == 3 || GateType == 4) && N->LogicOp == 'A' && N->Connectible[k]->ON * M == ZERO) ||
	       ((GateType == 3 || GateType == 4) && N->LogicOp == 'O' && N->Connectible[k]->OFF * M == ZERO) ||
	       (GateType == 3 && N->LogicOp == 'N' && N->Connectible[k]->ON * M == ZERO))
		Connectible.push_back(N->Connectible[k]);
	}
	return M;
    }
	
  //P: User Chooses - prompts used with heuristic H - Use only for NAND gates
  if(MintermChoice == 'P') {
      if(print) cout<<"Choose Node from {";
      map<int,Node*> NodeMap; //P
      int minInputs = INF; int minConn = INF;
      N = NULL;  
      for(int i = Internal.size()-1; i>=0; i--) { 
	  if(Internal[i]->Uncovered != ZERO) { 
	      if(print) cout<<" "<<Internal[i]; //P
 	      NodeMap[Internal[i]->idx] = Internal[i]; //P
	      if(N == NULL ||  
		 (Internal[i]->Inputs.size() < N->Inputs.size()) ||  
		 (Internal[i]->Inputs.size() == N->Inputs.size() && Internal[i]->Connectible.size() <= N->Connectible.size())) 
		  N = Internal[i]; 
	  }
      }
      for(int i = Outputs.size()-1; i>=0; i--) {                    
	  if(Outputs[i]->Uncovered != ZERO) {                       
	      if(print) cout<<" "<<Outputs[i];                      //P
 	      NodeMap[Outputs[i]->idx] = Outputs[i];                //P
	      if(N == NULL ||                                       
		 (Outputs[i]->Inputs.size() < N->Inputs.size()) ||  
		 (Outputs[i]->Inputs.size() == N->Inputs.size() && Outputs[i]->Connectible.size() <= N->Connectible.size()))
		  N = Outputs[i];
	  }
      }
      //P - begin
      if(print)	cout<<" }"<<endl;
      if(NodeMap.size() > 1) {
	  if(print) {
	      string mystr;
	      int index = -1;
	      while(index < INPUT || index > vars) {
		  cout<<"("<<N<<") > ";
		  getline(cin,mystr);
		  if(mystr != "")
		      index = stringToint(mystr);
		  else index = N->idx;
	      }
	      N = NodeMap[index];
	  }    
      }
      BDD ToCover = N->Uncovered;
      //P - end
      BDD M;
      if(print) {
	  cout<<"  Selected Node = "<<N<<endl;
	  cout<<"  ToCover = "; PrintFunction(ToCover); cout<<endl;
	  cout<<"Select Minterm"<<endl;
      }
      
      if(ToCover.CountMinterm(INPUT) == 1) 
	  M = ToCover;
      else {
	  if(print) cout<<"Pick minterm: "<<endl;
	  if(print) cout<<"  #: minterm - coverings"<<endl;
	  int minCovering = INF;
	  int minIndex = INF; 
	  DdNode* inputnodes[INPUT];
	  for(int i = 0; i < INPUT; i++)
	      inputnodes[i] = InputBdds[i].getNode();
	  BDDvector InputVector(INPUT, Manager, inputnodes);
	  map<int,BDD> MintermMap;  //P
	  while(ToCover != ZERO) {
	      BDD minterm = ToCover.PickOneMinterm(InputVector);
	      int Index = MintermIndex(minterm);
	      MintermMap[Index] = minterm;        //P
	      int ct = 0;  
	      for(int i = 0; i < N->Connectible.size(); i++)
		  if(minterm * !N->Connectible[i]->ON != ZERO) ct++;    
	      if(print) { cout<<"  "<<Index<<": "; PrintFunction(minterm); cout<<" - "<<ct; }
	      if(ct < minCovering) { 
		  M = minterm;
		  minCovering = ct; 
		  minIndex = Index;
		  if(print) cout<<" *";  //P
	      } else if(ct == minCovering) { 
		  if(Index < minIndex) {
		      M = minterm;
		      minCovering = ct; 
		      minIndex = Index;
		      if(print) cout<<" *";  //P
		  }
	      }
	      if(print) cout<<endl;
	      ToCover *= !minterm;
	  }
	  //P - begin
	  if(print) {
	      string mystr;
	      int index = -1;
	      while(index < 0) {
		  cout<<" > ";
		  getline(cin,mystr);
		  if(mystr != "")
		      index = stringToint(mystr);
		  else MintermIndex(M);
	      }
	      M = MintermMap[index];
	  }
	  //P - end
      }
      if(print) { cout<<"  Selected Minterm = "; PrintFunction(M); cout<<endl; }
      Connectible.clear();
      for(int k = 0; k < N->Connectible.size(); k++) {
	  if((GateType == 1 && N->Connectible[k]->ON * M == ZERO) ||
	     (GateType == 2 && N->Connectible[k]->OFF * M == ZERO) ||
	     ((GateType == 3 || GateType == 4) && N->LogicOp == 'A' && N->Connectible[k]->ON * M == ZERO) ||
	     ((GateType == 3 || GateType == 4) && N->LogicOp == 'O' && N->Connectible[k]->OFF * M == ZERO) ||
	     (GateType == 3 && N->LogicOp == 'N' && N->Connectible[k]->ON * M == ZERO))
	      Connectible.push_back(N->Connectible[k]);
      }
      return M;
  }  

  //S: Node: Choose highest level gate
  //   Minterm: Follow heuristic H
  //   ONLY GOOD FOR NAND GATES - USE THIS TO GET BEST SEARCH FOR NAND FUNCTIONS
  if(MintermChoice == 'S') {
      int maxLevel = -1;
      N = NULL;  
      for(int i = Internal.size()-1; i>=0; i--) { 
	  if(Internal[i]->Uncovered != ZERO) { 
	      if(Internal[i]->level > maxLevel) {
		  N = Internal[i]; 
		  maxLevel = N->level;
	      }
	  }
      }
      for(int i = Outputs.size()-1; i>=0; i--) {                    
	  if(Outputs[i]->Uncovered != ZERO) {                       
	      if(Outputs[i]->level > maxLevel) {
		  N = Outputs[i];
		  maxLevel = N->level;
	      }
	  }
      }
      BDD ToCover = N->Uncovered;
      BDD M;
      if(print) {
	  cout<<"  Selected Node = "<<N<<endl;
	  cout<<"  ToCover = "; PrintFunction(ToCover); cout<<endl;
	  cout<<"Select Minterm"<<endl;
      }
      
      if(ToCover.CountMinterm(INPUT) == 1) 
	  M = ToCover;
      else {
	  int minCovering = INF;
	  int minIndex = INF; 
	  DdNode* inputnodes[INPUT];
	  for(int i = 0; i < INPUT; i++)
	      inputnodes[i] = InputBdds[i].getNode();
	  BDDvector InputVector(INPUT, Manager, inputnodes);
	  while(ToCover != ZERO) {
	      BDD minterm = ToCover.PickOneMinterm(InputVector);
	      int Index = MintermIndex(minterm);
	      int ct = 0;  
	      for(int i = 0; i < N->Connectible.size(); i++)
		  if(minterm * !N->Connectible[i]->ON != ZERO) ct++;    
	      if(ct < minCovering) { 
		  M = minterm;
		  minCovering = ct; 
		  minIndex = Index;
	      } else if(ct == minCovering) { 
		  if(Index < minIndex) {
		      M = minterm;
		      minCovering = ct; 
		      minIndex = Index;
		  }
	      }
	      ToCover *= !minterm;
	  }
      }
      if(print) { cout<<"  Selected Minterm = "; PrintFunction(M); cout<<endl; }
      Connectible.clear();
      for(int k = 0; k < N->Connectible.size(); k++) {
	  if((GateType == 1 && N->Connectible[k]->ON * M == ZERO) ||
	     (GateType == 2 && N->Connectible[k]->OFF * M == ZERO) ||
	     ((GateType == 3 || GateType == 4) && N->LogicOp == 'A' && N->Connectible[k]->ON * M == ZERO) ||
	     ((GateType == 3 || GateType == 4) && N->LogicOp == 'O' && N->Connectible[k]->OFF * M == ZERO) ||
	     (GateType == 3 && N->LogicOp == 'N' && N->Connectible[k]->ON * M == ZERO))
	      Connectible.push_back(N->Connectible[k]);
      }
      return M;
  }
}

bool MoreOnes(BDD f) {
    int ones = 0;
    int total = 1<<INPUT;
    for(int i = 0; i < total; i++) {
	BDD M = MakeMinterm(InputBdds, i);
	if(f.Constrain(M) == ONE) ones++;
    }
    if(ones > total / 2) return true;
    else return false;
}

/* !!! */
BDD PickMinterm(BDD f) {
  if(INPUT > 6 || GateType > 2) {
    DdNode* inputnodes[INPUT];
    for(int i = 0; i < INPUT; i++)
      inputnodes[i] = InputBdds[i].getNode();
    BDDvector InputVector(INPUT, Manager, inputnodes);
    return f.PickOneMinterm(InputVector);
  } else {
      int pwr = 1<<INPUT;
      if(GateType == 1) {
	  for(int i = 0; i < pwr; i++) {
	      BDD M = MakeMinterm(InputBdds, i);
	      if(f * M != ZERO) return M;
	  }
      } else if(GateType == 2) {
	  for(int i = pwr-1; i >= 0; i--) {
	      BDD M = MakeMinterm(InputBdds, i);
	      if(f * M != ZERO) return M;
	  }
      }
  }  
}

/* !!! */
BDD SelectCover(BDD f, int integer) {
    BDD g = ZERO;
    for(int i = 0; i < (1<<INPUT); i++) {
	BDD M = MakeMinterm(InputBdds, i);
	if(f.Constrain(M) == ONE) {
	    if(integer % 2 == 1) {
		g += M;
	    }
	    integer /= 2;
	}
    }
    return g;
}

/* !!! */
BDD MakeMinterm(vector<BDD>& input, int integer) {
  BDD minterm = ONE;
  for (int i = input.size() - 1; i >= 0; i--) {
    if (integer % 2)
      minterm &= input[i];
    else 
      minterm &= !input[i];
    integer /= 2;
  }
  return minterm;
}

/* !!! */
void PrintInterval(BDD ON, BDD OFF, ostream& out) {
    if(InputBdds.size() > 6) {
      out<<"["<<ON.CountMinterm(INPUT)<<", "<<OFF.CountMinterm(INPUT)<<"]";
      return;
    }
    out<<"[";
    int total = 1<<InputBdds.size();
    int block = 1<<(InputBdds.size()/2);
    for(int i = 0; i < total; i++) {
	if(i % block == 0 && i != 0) out<<".";
	BDD M = MakeMinterm(InputBdds, i);
	if(ON.Constrain(M) == ONE && OFF.Constrain(M) == ONE) out<<"?";
	else if(ON.Constrain(M) == ONE) out<<"1";
	else if(OFF.Constrain(M) == ONE) out<<"0";
	else out<<"-";
    }
    out<<"]";
}
void PrintUncoveredFunction(Node* N, BDD F, ostream& out) {
    BDD uncovered;
    if(GateType == 1 || (GateType == 5 && N->LogicOp == 'D')) { uncovered = N->ON; for(int i = 0; i < N->Inputs.size(); i++) uncovered *= !N->Inputs[i]->OFF; }
    else if(GateType == 2 || (GateType == 5 && N->LogicOp == 'R')) { uncovered = N->OFF; for(int i = 0; i < N->Inputs.size(); i++) uncovered *= !N->Inputs[i]->ON; }
    else if((GateType == 3 || GateType == 4) && N->LogicOp == 'A') {uncovered = N->OFF; for(int i = 0; i < N->Inputs.size(); i++) uncovered *= !N->Inputs[i]->OFF; }
    else if((GateType == 3 || GateType == 4) && N->LogicOp == 'O') {uncovered = N->ON; for(int i = 0; i < N->Inputs.size(); i++) uncovered *= !N->Inputs[i]->ON; }
    else if(GateType == 3 && N->LogicOp == 'N') {uncovered = N->ON; for(int i = 0; i < N->Inputs.size(); i++) uncovered *= !N->Inputs[i]->OFF; }
    if(InputBdds.size() > 6) {
	out<<"Uncov: "<<uncovered.CountMinterm(INPUT);
	return;
    }
    int total = 1<<InputBdds.size();
    int block = 1<<(InputBdds.size()/2);
    out<<"[";
    for(int i = 0; i < total; i++) {
	BDD M = MakeMinterm(InputBdds, i);
      if(i % block == 0 && i != 0) out<<".";
      if(uncovered.Constrain(M) == ONE) {
	  if(F.Constrain(M) == ONE) {
	      if(GateType == 2 || N->LogicOp == 'O') out<<"0";
	      else out<<"1";
	  } else if(F.Constrain(M) == ZERO) out<<"*";
      } else if(uncovered.Constrain(M) == ZERO) {
	  if(F.Constrain(M) == ONE) out<<"?";
	  else if(F.Constrain(M) == ZERO) out<<"-";
      } else out<<"?";
    }   
    out<<"]";
}
/* !!! */
void PrintFunction(BDD F, ostream& out) {
  if(F == ONE) { out<<"1"; return; }
  if(F == ZERO) { out<<"0"; return; }
  if(InputBdds.size() > 6) {
      out<<"["<<F.CountMinterm(INPUT)<<", "<<!F.CountMinterm(INPUT)<<"]";
      return;
  }
  int total = 1<<InputBdds.size();
  int block = 1<<(InputBdds.size()/2);
  for(int i = 0; i < total; i++) {
      BDD M = MakeMinterm(InputBdds, i);
      if(i % block == 0 && i != 0) out<<".";
      if(F.Constrain(M) == ONE) out<<"1";
      else if(F.Constrain(M) == ZERO) out<<"0";
      else out<<"?";
  }
/*
  
vector<BDD> minterms;
  for(int i = 0; i < total; i++) {
    BDD M = MakeMinterm(InputBdds, i);
    if(F.Constrain(M) == ONE)
      minterms.push_back(M);
  }
  for(int i = 0; i < minterms.size(); i++) {
    for(int j = i+1; j < minterms.size(); j++) {
      int diff = 0;
      BDD term = ONE;
      for(int k = 0; k < InputBdds.size() && diff < 2; k++) {
	BDD posi = minterms[i].Constrain(InputBdds[k]);
	BDD posj = minterms[j].Constrain(InputBdds[k]);
	BDD negi = minterms[i].Constrain(!InputBdds[k]);
	BDD negj = minterms[j].Constrain(!InputBdds[k]);
	if(posi != posj && negi != negj && (posi == ZERO && negj == ZERO || negi == ZERO && posj == ZERO)) {
	  diff++;
	} else {
	  if(posi == ZERO || posj == ZERO) term = term & !InputBdds[k];
	  else if(negi == ZERO || negj == ZERO) term = term & InputBdds[k];
	}
      }
      if(diff == 1) {
	bool found = false;
	for(int k = 0; k < minterms.size() && !found; k++) 
	  if(minterms[k] == term) found = true;
	if(!found)
	  minterms.push_back(term);
      }
    }
  }
  vector<BDD> finalterms;
  for(int i = minterms.size()-1; i >= 0; i--) {
    bool found = false;
    for(int j = 0; j < finalterms.size() && !found; j++) {
      if(minterms[i] + finalterms[j] == finalterms[j]) found = true;
    }
    if(!found) finalterms.push_back(minterms[i]);
  }
  for(int i = 0; i < finalterms.size(); i++) {
    if(i != 0) out<<" + ";
    PrintTerm(finalterms[i],out);
  }
*/
}
/* !!! */
void PrintTerm(BDD M, ostream& out) {
  if(InputBdds.size() > 6) return;
  bool first = true;
  vector<BDD> tempBdds = InputBdds;
  for(int i = 0; i < tempBdds.size(); i++) {
    if(M.Constrain(tempBdds[i]) == ZERO) {
      if(first) first = false; else out<<".";
      if(i < INPUT) out<<char ('a'+i)<<"'";//out<<"x_"<<i<<"'";
      else out<<"v'";
    }
    if(M.Constrain(!tempBdds[i]) == ZERO) {
      if(first) first = false; else out<<".";
      if(i < INPUT) out<<char ('a'+i); //out<<"x_"<<i;
      else out<<"v";
    }
 }
}
//////////////////////////////////////////////////////////////////////////////
// Conversion Functions
//////////////////////////////////////////////////////////////////////////////
/* !!! */
string intTostring(int i) {
  if(i == 0) return "0";
  string s = "";
  while (i > 0) {
    char c = '0' + (i % 10);
    s = c + s;
    i /= 10;
  }
  return s;
}
/* !!! */
long stringToint (string s) {
  int ct = 0;
  long ans = 0;
  for(string::reverse_iterator i = s.rbegin(); i != s.rend(); i++) {
    int digit = 1;
    for(int j = 0; j<ct; j++)
      digit *= 10;
    switch ((*i)) {
    case '0':
      ans += digit * 0;
      break;
    case '1':
      ans += digit * 1;
      break;
    case '2':
      ans += digit * 2;
      break;
    case '3':
      ans += digit * 3;
      break;
    case '4':
      ans += digit * 4;
      break;
    case '5':
      ans += digit * 5;
      break;
    case '6':
      ans += digit * 6;
      break;
    case '7':
      ans += digit * 7;
      break;
    case '8':
      ans += digit * 8;
      break;
    case '9':
      ans += digit * 9;
      break;
    default:
      cout<<"Invalid character in function specification: "<<s<<endl;
      return 0;
    }
    ct++;
  }
  return ans;
}

string floatTostring(float f) {
  if(f == 0) return "0";
  string s = "";
  int i = (int) f;
  int ct = 1;
  while (i > 0) {
    char c = '0' + (i % 10);
    s = c + s;
    f = f - (i%10) * ct;
    i /= 10;
    ct *= 10;
  }
  if(f > 0) {
      s += ".";
      for(int j = 0; j < 3; j++) {
	  i = (int) (f * 10);
	  char c = '0' + (i % 10);
	  s = s+c;
	  f = (f*10) - i;
      }
  }
  return s;
}
/* !!! */
BDD intTobdd(int I) {
    BDD F = ZERO;
    for(int i = 0; i < (1<<INPUT); i++) {
	if(I % 2) {
	    F += MakeMinterm(InputBdds, (1<<INPUT)-i-1);
	}
	I /= 2;
    }
    return F;
}
/* !!! */
BDD stringTobdd(string s) {
  BDD F = ZERO;
  for(int i = 0; i < (1<<INPUT); i++)
    if(s[i] == '1')
      F += MakeMinterm(InputBdds, i);
  return F;
}
void ReadRandomFile(ifstream& ifile, vector<BDD>& Functions) {
  string line;
  getline(ifile, line); 
  bool firstTime = true;
  while(ifile) {
    int mid = line.find_first_of(":",0);
    string idxStr = line.substr(0, mid);
    string ftnStr = line.substr(mid+1, line.size()-mid-1);
    int idx = stringToint(idxStr);
    //if(idx > 71915) {  //TEMPORARY
    if(firstTime) {
	int pwr = ftnStr.size();
	INPUT = 1;
	while(pwr != 1<<INPUT) INPUT++;
	for(int i = 0; i < INPUT; i++)
	    InputBdds.push_back(Manager->bddVar());
	firstTime = false;
    }
    BDD ftn = stringTobdd(ftnStr);
    if(Number(ftn) != idx) {
	cout<<"Incorrect function? "<<idx<<" != "; PrintFunction(ftn); cout<<endl;
    }
    //Functions.insert(Functions.begin(),ftn);  //TEMPORARY
    Functions.push_back(ftn);  //ADD
    //}
    getline(ifile, line);
  }
}
/* !!! */
void ReadBlifFile(ifstream& blif, vector<BDD>& OutputBdds) {
    string line;
    BDD f = ZERO;
    map<string,BDD> VariableMap;
    map<string,BDD> InputMap;
    map<string,BDD> OutputMap;
    map<string,BDD> FunctionValue;
    getline(blif,line);
    vector<BDD> CurrentInBdds;
    vector<string> CurrentInVars;
    BDD CurrentOutBdd;
    string CurrentOutVar;
    while(blif) {
	//cout<<"line = "<<line<<endl;
	if(line.find(".model") != string::npos) {

	} else if(line.find(".inputs") != string::npos) {
	    int begin = line.find_first_of(" ",0)+1;
	    int counter = 0;
	    while(begin != string::npos) {
		int end = line.find_first_of(" ",begin);
		string var = line.substr(begin,end-begin);
		//cout<<"\tInput Variable "<<counter<<" = "<<var<<endl;
		BDD in = Manager->bddVar();
		VariableMap[var] = in;
		InputMap[var] = in;
		begin = line.find_first_not_of(" ", end);
		counter++;
	    }
	} else if(line.find(".outputs") != string::npos) {
	    int begin = line.find_first_of(" ",0)+1;
	    int counter = 0;
	    while(begin != string::npos) {
		int end = line.find_first_of(" ",begin);
		string var = line.substr(begin,end-begin);
		//cout<<"\tOutput Variable "<<counter<<" = "<<var<<endl;
		VariableMap[var] = Manager->bddVar();
		OutputMap[var] = VariableMap[var];
		begin = line.find_first_not_of(" ", end);
		counter++;
	    }
	} else if(line.find(".names") != string::npos) {
	    if(!CurrentInVars.empty()) {
		FunctionValue[CurrentOutVar] = f;
		/*cout<<"\t\t"; for(int i = 0; i < CurrentInVars.size(); i++) cout<<CurrentInVars[i]<<" "; cout<<endl;
		cout<<"\t"<<CurrentOutVar<<" = ";
		for(int i = 0; i < (1<<CurrentInBdds.size()); i++) {
		    BDD M = MakeMinterm(CurrentInBdds, i);
		    if(f.Constrain(M) == ONE) cout<<"1";
		    else cout<<"0";
		}
		cout<<endl;*/
	    }
	    CurrentInBdds.clear();
	    CurrentInVars.clear();
	    int begin = line.find_first_of(" ",0)+1;
	    while(begin != string::npos) {
		int end = line.find_first_of(" ",begin);
		string var = line.substr(begin,end-begin);
		//cout<<"\tCurrent Variable = "<<var<<endl;
		if(VariableMap.find(var) == VariableMap.end())
		    VariableMap[var] = Manager->bddVar();
		if(end != string::npos) {
		    CurrentInBdds.push_back(VariableMap[var]);
		    CurrentInVars.push_back(var);
		} else {
		    CurrentOutBdd = VariableMap[var];
		    CurrentOutVar = var;
		}
		begin = line.find_first_not_of(" ", end);
	    }
	    f = ZERO;
	} else if(line.find(".end") != string::npos) {
	    if(!CurrentInVars.empty()) {
		FunctionValue[CurrentOutVar] = f;
		/*cout<<"\t\t"; for(int i = 0; i < CurrentInVars.size(); i++) cout<<CurrentInVars[i]<<" "; cout<<endl;
		cout<<"\t"<<CurrentOutVar<<" = ";
		for(int i = 0; i < (1<<CurrentInBdds.size()); i++) {
		    BDD M = MakeMinterm(CurrentInBdds, i);
		    if(f.Constrain(M) == ONE) cout<<"1";
		    else cout<<"0";
		}
		cout<<endl;*/
	    }
	    vector<bool> UsedInput(InputMap.size(), false);
	    vector<BDD> OutputFunctions;
	    for(map<string,BDD>::iterator i = OutputMap.begin(); i != OutputMap.end(); i++) {
		string var = (*i).first;
		BDD val = (*i).second;
		f = FunctionValue[var];
		bool changes = true;
		while(changes) {
		    changes = false;
		    for(map<string,BDD>::iterator j = VariableMap.begin(); j != VariableMap.end(); j++) {
			string varj = (*j).first;
			if(InputMap.find(varj) == InputMap.end()) {
			    BDD valj = (*j).second;
			    if(f.Constrain(valj) != f.Constrain(!valj)) {
				f = (f.Constrain(valj)*FunctionValue[varj]) + (f.Constrain(!valj)*!FunctionValue[varj]);
				changes = true;
				j = VariableMap.end();
			    }
			}
		    }
		}
		//cout<<var<<" = "; PrintFunction(f); cout<<endl;
		//if(print) { cout<<"FUNCTION: "; PrintFunction(f); cout<<endl; }
		OutputFunctions.push_back(f);
		int ct = 0;
		for(map<string,BDD>::iterator j = InputMap.begin(); j != InputMap.end(); j++, ct++)
		    if(!UsedInput[ct])
			if(f.Constrain((*j).second) != f.Constrain(!(*j).second)) UsedInput[ct] = true;
	    }
	    INPUT = 0;
	    int ct = 0;
	    for(map<string,BDD>::iterator i = InputMap.begin(); i != InputMap.end(); i++, ct++)
		if(UsedInput[ct]) {
		    InputBdds.push_back((*i).second);
		    INPUT++;
		    //cout<<"INPUT: "<<(*i).first<<" - "; PrintFunction((*i).second); cout<<endl;
		}
	    for(int i = 0; i< OutputFunctions.size(); i++) {
		//cout<<"OUTPUT: "; PrintFunction(OutputFunctions[i]); cout<<"  ";
		bool EqualsInput = false;
		for(int j = 0; j < InputBdds.size() && !EqualsInput; j++)
		    if(OutputFunctions[i] == InputBdds[j]) EqualsInput = true;
		if(!EqualsInput) OutputBdds.push_back(OutputFunctions[i]);    
		//else cout<<"__";
		//cout<<endl;
	    }
	    return;
	} else if(line[0] == '1' || line[0] == '0' || line[0] == '-'){
	    BDD c = ONE;
	    int i;
	    for(i = 0; line[i] != ' '; i++) {
		if(line[i] != ' ')
		    if(line[i] == '1')
			c *= CurrentInBdds[i];
		    else if(line[i] == '0')
			c *= !CurrentInBdds[i];
	    }
	    /*cout<<"\tc = ";
	    for(int j = 0; j < (1<<CurrentInBdds.size()); j++) {
	      BDD M = MakeMinterm(CurrentInBdds, j);
	      if(c.Constrain(M) == ONE) cout<<"1";
	      else cout<<"0";
	    }
	    cout<<endl;*/
	    while(line[i] == ' ') i++;
	    if(line[i] == '1') f += c;
	    else if(line[i] == '0') {
		if(f == ZERO) f = !c;
		else f *= !c;
	    } 
	    /*cout<<"\tf = ";
	    for(int j = 0; j < (1<<CurrentInBdds.size()); j++) {
		BDD M = MakeMinterm(CurrentInBdds, j);
		if(f.Constrain(M) == ONE) cout<<"1";
		else cout<<"0";
	    }
	    cout<<endl;*/
	}
	getline(blif, line);
    }
}

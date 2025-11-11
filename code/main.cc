#include "print.h"
#include <time.h>
#include <cctype>

//Cost bounds
float UpperBound;

//Some Stats
int More2Width;
int minWidth, maxWidth, ctWidth;
float avgWidth;
int minHeight, maxHeight, ctHeight;
float avgHeight;

int DuplCoverCount;
int symmetryCount;
int GateSymmCount;

//Keeps track of completed functions
vector<BDD> Reps;
vector<double> RepSize;
vector<int> RepInput;

//Timer
time_t begin;

//Infinite network Stopper
bool STOP;
bool SetTreeFirst; //&&

map<int, vector<Network*> > AllNetworks;

bool InitialTreeStructure;
int InitialCost;
int OptimalNodeNum;


bool CheckPerm(BDD F, BDD G) {

  vector<bool> bv(1<<INPUT);
  for(long j = 0; j < (1<<INPUT); j++) {
    BDD M = MakeMinterm(InputBdds,j);
    bv[j] = (F.Cofactor(M) == ONE);
  }

  vector<BDD> perm(INPUT);
  int end = 1;
  for(int j = 0; j < INPUT; j++) {
    end *= (j+1);
    perm[j] = InputBdds[j];
  }
  
  for(int j = 0; j < end; j++) {
    BDD H = ZERO;
    for(long k = 0; k < (1<<INPUT); k++) {
      if(bv[k]) {
	BDD M = MakeMinterm(perm, k);
	H += M;
      }
    }
    if(G == H) return true;

    BDD temp;
    int j_end = 1;
    for(int k = 1; k < INPUT; k++) {
      if(j % j_end == 0) {
	temp = perm[k];
	for(int l = k; l > 0; l--)
	  perm[l] = perm[l-1];
	perm[0] = temp;
      }
      j_end *= (k+1);
    }
  }
  return false;  
}

bool NewFunction (BDD f) {
  double size = f.CountMinterm(INPUT);
  int input = 0;
  for(int j = 0; j < INPUT; j++) {
    BDD pos = f.Constrain(InputBdds[j]);
    BDD neg = f.Constrain(!InputBdds[j]);
    if(pos != neg) input++;
  }
  bool found = false;
  for(int j = 0; j < Reps.size() && !found; j++) {
    if(size != RepSize[j]) continue;
    if(input != RepInput[j]) continue;
    set<double> FCount;
    set<double> RepCount;
    for(int k = 0; k < INPUT; k++) {
      BDD pos = f.Constrain(InputBdds[k]);
      BDD neg = f.Constrain(!InputBdds[k]);
      FCount.insert(pos.CountMinterm(INPUT));
      FCount.insert(neg.CountMinterm(INPUT));
      pos = Reps[j].Constrain(InputBdds[k]);
      neg = Reps[j].Constrain(!InputBdds[k]);
      RepCount.insert(pos.CountMinterm(INPUT));
      RepCount.insert(neg.CountMinterm(INPUT));
    }
    if(FCount != RepCount) continue;
    if(CheckPerm(Reps[j],f))
      found = true;
  }
  if(!found) {
    Reps.push_back(f);
    RepSize.push_back(size);
    RepInput.push_back(input);
  } //else { cout<<"Skip: "; PrintFunction(f); cout<<endl; }
  return !found;
}

bool Dependents(BDD f) {
  for(int i = 0; i < INPUT; i++) {
    BDD pos = f.Constrain(InputBdds[i]);
    BDD neg = f.Constrain(!InputBdds[i]);
    if(pos == neg) return false;
  }
  return true;
}

bool IsSolution(Network* Net) {
    for(int i = 0; i < Solutions.size(); i++) {
	if(Solutions[i] != NULL && Solutions[i]->GetIdx() == Net->GetIdx())
	    return true;
    }
    return false;
}
void DeleteNetwork(Network* Net) {
    if(!IsSolution(Net))
	delete Net;
}

void UpdateStats(int value, bool type) {
    if(type) {
      //height Stats
      if(value > maxHeight) maxHeight = value;
      if(value < minHeight) minHeight = value;
      if(ctHeight == 0) avgHeight = value;
      else avgHeight = (avgHeight * ctHeight + value)/(ctHeight+1); 
      ctHeight++;
    } else {
      //width Stats
	if(value > 2) More2Width++;
	if(value > maxWidth) maxWidth = value;
	if(value < minWidth) minWidth = value; 
	if(ctWidth == 0) avgWidth = value;
	else   avgWidth = (avgWidth * ctWidth + value)/(ctWidth+1); 
	ctWidth++;
    }
}

void InsertNetwork(vector<pair<Network*, Node*> >& CompletedNetworks, Network* Net, Network* OldNet, Node* C, Node* N, set<Node*>& BlankNodes, Node* newC) {
    vector<pair<Network*,Node*> >::iterator i = CompletedNetworks.begin();
    for( ; i != CompletedNetworks.end() && (*i).first->Cost() <= Net->Cost(); i++);
    CompletedNetworks.insert(i, pair<Network*,Node*> (Net, C));
}

vector<vector<int> > SuboptDetails;

void ComputeNetwork(Network* Net, int iteration) {

  ComputeNetworkCount++;

  if(UpperBound < INF && RelaxVer == 'C' && ComputeNetworkCount % RelaxConst == 0) {
      UpperBound = UpperBound - RelaxPercent;
      RelaxDone++;
  }

  if(UpperBound < INF && RelaxVer == 'E' && ComputeNetworkCount % RelaxConst == 0){
      //cout<<"Relax "<<1-RelaxPercent<<" every "<<RelaxConst<<" branches"<<endl; 
      //cout<<"Upper Bound("<<RelaxDone<<"): "<<UpperBound<<" -> ";
      status<<"Upper Bound("<<RelaxDone<<"): "<<UpperBound<<" -> ";
      UpperBound *= (1 - RelaxPercent);
      //cout<<UpperBound<<endl;
      status<<UpperBound<<endl;
      RelaxDone++;
  }

  PrintIteration(iteration);

  //int SimpleNetworks = Solutions.size();

  //if(iteration % 10 == 0)  {
  //    cout<<iteration<<": "<<Net->Cost()<<": "<<ComputeNetworkCount<<": "<<time(NULL)-begin<<endl;
  //}

  if(iteration % 100 == 0 && Solutions.empty())  {
      status<<iteration<<": "<<Net->Cost()<<endl;
      
      //Net->PrintText();
      cout<<iteration<<": "<<Net->Cost()<<endl;
      //cout<<"> "; cin.get();
      
  }

  if(iteration == 8000 && Solutions.empty()) {
      STOP = true;
      DeleteNetwork(Net);
      InitialCost = Net->Cost();
      OptimalNodeNum = ComputeNetworkCount;
      //cout<<"Return, iteration = 8000"<<endl;  //TEMP
      return;
  }
 /** If every node is covered then Net is a solution **/
  if(!Net->UncoveredNode()) {
      LeafCount++;
      if(Solutions.empty()) {
	  if(Net->TreeStructure()) InitialTreeStructure = true;
	  else InitialTreeStructure = false;
	  InitialCost = Net->Cost();
	  if(RelaxVer > 'A') {
	      SuboptDetails[0].clear();
	      SuboptDetails[0].push_back(Net->Cost());
	      SuboptDetails[0].push_back(Net->Gates());
	      SuboptDetails[0].push_back(Net->Level());
	      SuboptDetails[0].push_back(Net->Edges());
	      SuboptDetails[0].push_back(time(NULL)-begin);
	      SuboptDetails[0].push_back(ComputeNetworkCount);
	      SuboptDetails[0].push_back(StrucImplCount);
	      SuboptDetails[1] = SuboptDetails[0];
	      SuboptDetails[2] = SuboptDetails[0];
	      SuboptDetails[3] = SuboptDetails[0];
	      SuboptDetails[4] = SuboptDetails[0];
	      SuboptDetails[5] = SuboptDetails[0];
	      SuboptDetails[6] = SuboptDetails[0];
	  }
      }
      if(Net->Cost() <= UpperBound || (RelaxVer > 'A' && (Solutions.empty() || Net->Cost() <= Solutions[0]->Cost()))) {
	  if((Net->Cost() < UpperBound) || 
	     (RelaxVer > 'A' && (Solutions.empty() || Net->Cost() < Solutions[0]->Cost())) ||
	     (!LevelBoundCheck && onesol) || 
	     (LevelBoundCheck && (Net->Level() < LevelBound || (Net->Level() == LevelBound && onesol)))) {
		 if(RelaxVer == 'A' || RelaxVer == 'C' || RelaxVer == 'E' || RelaxVer == 'F') UpperBound = Net->Cost();
		 else if(RelaxVer == 'B') UpperBound = Net->Cost()-RelaxConst;
		 else if(RelaxVer == 'D') UpperBound = Net->Cost()*(1-RelaxPercent);
		 LevelBound = Net->Level();
		 for(int i = 0; i < Solutions.size(); i++) {
		     Network* tempNet = Solutions[i];
		     Solutions[i] = NULL;
		     DeleteNetwork(tempNet);
		 }
		 Solutions.clear();
		 OptimalNodeNum = ComputeNetworkCount;
	  }
	  if(!onesol && Net->SymmetricSol(Solutions)) {
	      DeleteNetwork(Net);
	      //cout<<"Return: Solution (1)"<<endl; //TEMP
	      return;
	  }
	  //if(!Net->Duplicates()) {
	  time_t tempTime = time (NULL);
	  PrintIterationSolution(iteration, UpperBound, tempTime-begin, Net);
	  Solutions.push_back(Net);
	  if(RelaxVer > 'A') {
	      int idx;
	      if(time(NULL) - begin <= 60 * 60) idx = 1;
	      else if(time(NULL) - begin <= 60 * 60 * 3) idx = 2;
	      else if(time(NULL) - begin <= 60 * 60 * 5) idx = 3;
	      else if(time(NULL) - begin <= 60 * 60 * 10) idx = 4;
	      else if(time(NULL) - begin <= 60 * 60 * 24) idx = 5;
	      else idx = 6;
	      SuboptDetails[idx].clear();
	      SuboptDetails[idx].push_back(Net->Cost());
	      SuboptDetails[idx].push_back(Net->Gates());
	      SuboptDetails[idx].push_back(Net->Level());
	      SuboptDetails[idx].push_back(Net->Edges());
	      SuboptDetails[idx].push_back(time(NULL)-begin);
	      SuboptDetails[idx].push_back(ComputeNetworkCount);
	      SuboptDetails[idx].push_back(StrucImplCount);
	      for(int i_idx = idx+1; i_idx < 7; i_idx++)
		  SuboptDetails[i_idx] = SuboptDetails[idx];
	  }
	  //}
      }
      
      //height Stats
      UpdateStats(iteration, true);
      
      if(FirstNetwork) {
	  STOP = true;
      }
      //cout<<"Return: Solution (2)"<<endl; //TEMP
      return;
  }

  /*if((time(NULL) - begin > StopTime) || (RelaxVer == 'F' && (time(NULL) - begin) > RelaxConst)) {
      if(print) cout<<"STOP HERE: "<<iteration<<endl;
      STOP = true;
      DeleteNetwork(Net);
      //cout<<"Return: Version F"<<endl; //TEMP
      return;
      }*/

  //Find the Minterm and Node for covering & the Connectible nodes that can do the covering.
  Node* N;
  vector<Node*> Connections;
  BDD SelectedMinterm = Net->SelectMintermForCovering(N, Connections);
  PrintToCover(N, SelectedMinterm);
  bool FirstTime = true;
  bool TwoInputs = true;

  /*if(iteration % 25 == 0) {
      cout<<"Iter = "<<iteration<<endl;
      cout<<"  Cost - "<<Net->Cost()<<endl;
      cout<<"  Cover "<<N<<" = "; PrintInterval(N->ON, N->OFF); cout<<"  "; PrintUncoveredFunction(N, N->Uncovered); cout<<endl;
      cout<<"  Remaining: "<<Net->CountUncovered()<<endl;
      }*/

  /** Add a Blank node to the Connections set as long as N has an open input for it **/
  set<Node*> BlankNodes;
  if(N->Inputs.size() < FaninBound) {
      if(GateType == 1 || GateType == 2) {
	  Node* BlankNode = newNode();
	  Connections.push_back(BlankNode);
	  BlankNodes.insert(BlankNode);
	  TwoInputs = false;
      } else if(GateType == 5) {
	if(N->LogicOp == 'D') {
	  Node* BlankNode = newNode('D');
	  Connections.push_back(BlankNode);
	  BlankNodes.insert(BlankNode);
	  TwoInputs = false;
	  BlankNode = newNode('R');
	  Connections.push_back(BlankNode);
	  BlankNodes.insert(BlankNode);
	  TwoInputs = false;
	} else if(N->LogicOp == 'R') {
	  Node* BlankNode = newNode('R');
	  Connections.push_back(BlankNode);
	  BlankNodes.insert(BlankNode);
	  TwoInputs = false;
	  BlankNode = newNode('D');
	  Connections.push_back(BlankNode);
	  BlankNodes.insert(BlankNode);
	  TwoInputs = false;
	}
      } else if(GateType == 3 || GateType == 4) {
	  if(N->LogicOp == 'A') {
	      Node* BlankNode = newNode('O');
	      Connections.push_back(BlankNode);
	      BlankNodes.insert(BlankNode);
	      if(GateType == 3) {
		  BlankNode = newNode('N');
		  Connections.push_back(BlankNode);
		  BlankNodes.insert(BlankNode);
	      }
	      BlankNode = newNode('A');
	      Connections.push_back(BlankNode);
	      BlankNodes.insert(BlankNode);
	  } else if(N->LogicOp == 'O') {
	      Node* BlankNode;
	      if(GateType == 3) {
		  BlankNode = newNode('N');
		  Connections.push_back(BlankNode);
		  BlankNodes.insert(BlankNode);
	      }
	      BlankNode = newNode('A');
	      Connections.push_back(BlankNode);
	      BlankNodes.insert(BlankNode);
	      BlankNode = newNode('O');
	      Connections.push_back(BlankNode);
	      BlankNodes.insert(BlankNode);
	  } else if(N->LogicOp == 'N') {
	      Node* BlankNode = newNode('A');
	      Connections.push_back(BlankNode);
	      BlankNodes.insert(BlankNode);
	      BlankNode = newNode('O');
	      Connections.push_back(BlankNode);
	      BlankNodes.insert(BlankNode);
	      BlankNode = newNode('N');
	      Connections.push_back(BlankNode);
	      BlankNodes.insert(BlankNode);
	  }
      }
  }

  PrintConnectible(SelectedMinterm, Connections);

  /** Remove all but one of the input nodes from a symmetric set **/
  map<Node*, vector<Node*> >Removed;
  if(symmetry) Connections = Net->RemoveSymmetric(N, Connections, Removed);
  if(!Removed.empty()) symmetryCount++;

  //If no solution found yet, make the network a tree
  //First node in connectible set should be an input node, fan-in node, or new gate
  if(TreeFirst && Solutions.empty() && (NetworkOrder == 'A' || NetworkOrder == 'B' || NetworkOrder == 'F' || GateType == 4 || GateType == 5)) {
      if(!(Net->IsInput(Connections[0]) || N->IsFanin(Connections[0]) || BlankNodes.find(Connections[0]) != BlankNodes.end())) {
	  //Put Input node first
	  int i = 0;
	  for(;i < Connections.size(); i++) if(Net->IsInput(Connections[i])) break;
	  if(i < Connections.size()) {
	      Node* Temp1 = Connections[0];
	      Connections[0] = Connections[i];
	      for(int j = 1; j <= i; j++) {
		  Node* Temp2 = Connections[j];
		  Connections[j] = Temp1;
		  Temp1 = Temp2;
	      }
	  } else {
	      //Put Fan-in node first
	      i = 0;
	      for(;i < Connections.size(); i++) if(N->IsFanin(Connections[i])) break;
	      if(i < Connections.size()) {
		  Node* Temp1 = Connections[0];
		  Connections[0] = Connections[i];
		  for(int j = 1; j <= i; j++) {
		      Node* Temp2 = Connections[j];
		      Connections[j] = Temp1;
		      Temp1 = Temp2;
		  }
	      } else {
		  //Put New gate first
		  i = 0;
		  for(;i < Connections.size(); i++) if(BlankNodes.find(Connections[i]) != BlankNodes.end()) break;
		  if(i < Connections.size()) {
		      Node* Temp1 = Connections[0];
		      Connections[0] = Connections[i];
		      for(int j = 1; j <= i; j++) {
			  Node* Temp2 = Connections[j];
			  Connections[j] = Temp1;
			  Temp1 = Temp2;
		      }
		  }
		  
	      }
	  }
      }
  }

  //PrintConnectible(SelectedMinterm, Connections);
  
  //Count the number of networks we're creating to determine whether we have a Branching iteration or an Implication iteration
  int NetworksCreated = 0;
  
  //Need these if we're generating all networks before recursive calls are made.
  vector<pair<Network*, Node*> > CompletedNetworks;
  vector<int> Costs;                                
  
  /** Call ComputeNetwork based on the order of Connections.  The Connections set is ordred according to the variable: NetworkOrder **/
  for(int i = 0; i < Connections.size(); i++) {
      Node* C = Connections[i];
      /** Print the details of the current connection **/
      if(!FirstTime && (NetworkOrder == 'A' || NetworkOrder == 'B' || NetworkOrder == 'F' || GateType == 4)) { 
	  PrintShortDetails(iteration, N, SelectedMinterm); 	
	  PrintConnectible(SelectedMinterm, Connections);
      }
    PrintBranch(N, C);
    if(NetworkOrder == 'A' || NetworkOrder == 'B' || NetworkOrder == 'F' || GateType == 4) FirstTime = false;
    
    /** Perform Covering & Update On/Off-sets in the Network **/
    map<Node*,Node*> Mapping;
    Network* NewNetwork = Net->AddNodeInput(N, C, SelectedMinterm, Mapping);
    
    /** Continue as long as this connection created a valid network **/
    if(NewNetwork != NULL) {
	NetworksCreated++;
	Node* NewC = Mapping[C]; Node* NewN = Mapping[N];

	/** the new network must be smaller than the current minimum **/
	if((NewNetwork->LowerBound() < UpperBound) || 
	   ((NewNetwork->LowerBound() == UpperBound) &&
	    ((!LevelBoundCheck && !onesol) || 
	     (LevelBoundCheck && (NewNetwork->Level() < LevelBound || (NewNetwork->Level() == LevelBound && !onesol)))))) {
	    //if(NewNetwork->Cost() < UpperBound || (!onesol && NewNetwork->Cost() == UpperBound)) {
	    /** Do structural implications by adding new gates now **/
	    if(StrucImpl && GateType != 3 && GateType != 4 && GateType != 5) NewNetwork->AddNewNodes();
	    if((NewNetwork->LowerBound() < UpperBound) || 
	       ((NewNetwork->LowerBound() == UpperBound) &&
		((!LevelBoundCheck && !onesol) || 
		 (LevelBoundCheck && (NewNetwork->Level() < LevelBound || (NewNetwork->Level() == LevelBound && !onesol)))))) {
		//if(NewNetwork->Cost() < UpperBound || (!onesol && NewNetwork->Cost() == UpperBound)) {
		//** If using level restriction, make sure network is not over **/
		if(!LevelRestrict || NewNetwork->Level() <= LevelRestriction) {
		    //NewNetwork->PrintPicture();
		    if(NetworkOrder == 'A' || NetworkOrder == 'B'|| NetworkOrder == 'F' || GateType == 4) {	
			PrintEnd(NewNetwork, NewN, iteration);
			ComputeNetwork(NewNetwork, iteration+1);
			if(STOP) {
			    if(NetworksCreated > 1 || Connections.size() > 1) BranchCount++;
			    else ImplicationCount++;
			    //width Stats
			    UpdateStats(Connections.size(), false);
			    DeleteNetwork(Net);
			    if(!BlankNodes.empty())  for(set<Node*>::iterator b = BlankNodes.begin(); b != BlankNodes.end(); b++) delete (*b);
			    //cout<<"Return: STOP"<<endl;  //TEMP
			    return;
			}
		    } else if(NetworkOrder == 'C' || NetworkOrder == 'D'|| NetworkOrder == 'E') {
			InsertNetwork(CompletedNetworks, NewNetwork, Net, C, N, BlankNodes, NewC);
			if(print) {
			    cout<<" "<<NewNetwork->GetIdx()<<"("<<NewNetwork->LowerBound()<<") -> { ";
			    for(int i = 0; i < CompletedNetworks.size(); i++) {
				if(i != 0) cout<<", ";
				cout<<CompletedNetworks[i].first->GetIdx()<<"("<<CompletedNetworks[i].first->LowerBound()<<")";
			    }
			    cout<<" }"<<endl;
			}
		    }
		} else { //Level > LevelRestriction
		    LeafCount++;
		    ErrorPrint(intTostring(NewNetwork->Level()) + " > " + floatTostring(LevelRestriction) + " -> Next", NewNetwork, NewN, iteration);
		    DeleteNetwork(NewNetwork);
		    //height Stats
		    UpdateStats(iteration+1, true);
		}
	    } else {  //Cost > UpperBound
		//if(Solutions.empty())
		//    cout<<NewNetwork->GetNumber()<<" prune based on artificial UpperBound"<<endl;
		LeafCount++;
		ErrorPrint(intTostring(NewNetwork->Cost()) + " >= " + floatTostring(UpperBound) + " -> Next", NewNetwork, NewN, iteration);
		DeleteNetwork(NewNetwork);
		//height Stats
		UpdateStats(iteration+1, true);
	    }
	} else {  //Cost > UpperBound
	    //if(Solutions.empty())
	    //cout<<NewNetwork->GetNumber()<<" prune based on artificial UpperBound"<<endl;
	    LeafCount++;
	    ErrorPrint(intTostring(NewNetwork->LowerBound()) + " >= " + floatTostring(UpperBound) + " -> Next", NewNetwork, NewN, iteration);
	    DeleteNetwork(NewNetwork);
	    //height Stats
	    UpdateStats(iteration+1, true);
	}
    } else {  //NewNetwork = NULL
	ErrorPrint("Error creating new network", NULL, N, iteration);
	/*if(Net->Cost() < 10) {
	    Net->PrintPicture();
	    cout<<"iteration = "<<iteration<<endl;
	    cout<<"Cover "; PrintFunction(SelectedMinterm); cout<<" from "<<N<<" with "<<C<<endl;
	    cin.get();
	    }*/
	//height Stats
	UpdateStats(iteration+1, true);
	//&&
	if(Solutions.empty() && TreeFirst == false && UpperBound >= INF) {
	    SetTreeFirst = true;
	    STOP = true;
	    STOP = true;
	    DeleteNetwork(Net);
	    if(!BlankNodes.empty())  for(set<Node*>::iterator b = BlankNodes.begin(); b != BlankNodes.end(); b++) delete (*b);
	    //cout<<"Return: TreeFirst"<<endl; //TEMP
	    return;
	}
        //&&
    }
    /** Add SelectedMinterm to C's on-set **/
    if(DuplCover) {
	if(FindInput(N,C) != -1) {
	    if(print && GateType == 1) { cout<<iteration<<": Add "; PrintFunction(SelectedMinterm); cout<<" to "<<C<<"'s on-set"<<endl; }
	    if(print && GateType == 2) { cout<<iteration<<": Add "; PrintFunction(SelectedMinterm); cout<<" to "<<C<<"'s off-set"<<endl; }
	    if((GateType == 1 && !UpdateOnSet(C, SelectedMinterm, NULL, 0)) ||
	       (GateType == 2 && !UpdateOffSet(C, SelectedMinterm, NULL, 0))){
		LeafCount += Connections.size() - i - 1;
		if(GateType == 1) ErrorPrint("Error updating on-set of " + C->Name(), Net, N, iteration);
		else if(GateType == 2) ErrorPrint("Error updating off-set of " + C->Name(), Net, N, iteration);
		//height Stats
		UpdateStats(iteration+1, true);
		i = Connections.size();
		DuplCoverCount++;
	    }
	}
    }
    /** Add C to N's AlreadyConnected set **/
    if(GateSymm) {
	if(FindInput(N, C) == -1 && 
	   ((GateType == 1 && C->OFF * SelectedMinterm == SelectedMinterm) ||
	    (GateType == 2 && C->ON * SelectedMinterm == SelectedMinterm))) {
	    N->AlreadyConnected.insert(C);
	    GateSymmCount++;
	    if(Net->IsInput(C) && C->Parents.empty()) {
		set<Node*> Symmetric = Net->Symmetric(C);
		for(set<Node*>::iterator s = Symmetric.begin(); s != Symmetric.end(); s++) 
		    if((*s)->Parents.empty()) {
			N->AlreadyConnected.insert(*s);
			GateSymmCount++;
		    }
	    }
	}
    }
  } // end for loop over Connections
  
  if(GateType != 4 && (NetworkOrder == 'C' || NetworkOrder == 'D'|| NetworkOrder == 'E')) {
      if(UpperBound == INF && TreeFirst && Solutions.empty() && CompletedNetworks.size() > 1) {
	  int i = 0;
	  Node* First = CompletedNetworks[i].second;
	  while(!Net->IsInput(First) && !N->IsFanin(First) && BlankNodes.find(First) == BlankNodes.end()) {
	      i++;
	      First = CompletedNetworks[i].second;
	  }
	  pair<Network*, Node*> FirstPair = CompletedNetworks[i];
	  for(int j = i; j > 0; j--)
	      CompletedNetworks[j] = CompletedNetworks[j-1];
	  CompletedNetworks[0] = FirstPair;
      }
      if(print) {
	  cout<<"Final Order: {";
	  for(int i = 0; i < CompletedNetworks.size(); i++) {
	      if(i != 0) cout<<", ";
	      cout<<CompletedNetworks[i].first->GetIdx()<<"("<<CompletedNetworks[i].first->LowerBound()<<")";
	  }
	  cout<<" }"<<endl;
      }
      for(int i = 0; i < CompletedNetworks.size(); i++) {
	  if(!FirstTime) {
	      PrintShortDetails(iteration, N, SelectedMinterm);
	      PrintConnectible(SelectedMinterm, Connections);
	  }
	  PrintBranch(N, CompletedNetworks[i].second);
	  FirstTime = false;
	  if((CompletedNetworks[i].first->LowerBound() < UpperBound) || 
	     ((CompletedNetworks[i].first->LowerBound() == UpperBound) &&
	      ((!LevelBoundCheck && !onesol) || 
	       (LevelBoundCheck && (CompletedNetworks[i].first->Level() < LevelBound || (CompletedNetworks[i].first->Level() == LevelBound && !onesol)))))) {

	      //if(CompletedNetworks[i].first->Cost() < UpperBound || (!onesol && CompletedNetworks[i].first->Cost() == UpperBound)) {
	      PrintEnd(CompletedNetworks[i].first, N, iteration);
	      ComputeNetwork(CompletedNetworks[i].first, iteration+1);
	      if(STOP) {
		  for(int j = i+1; j < CompletedNetworks.size(); j++)
		      DeleteNetwork(CompletedNetworks[j].first);
		  break;
	      }
	  } else {  //Cost > UpperBound
	      //if(Solutions.empty())
	      //cout<<CompletedNetworks[i].first->GetNumber()<<" prune based on artificial UpperBound"<<endl;
	      LeafCount++;
	      ErrorPrint(intTostring(CompletedNetworks[i].first->LowerBound()) + " >= " + floatTostring(UpperBound) + " -> Next", CompletedNetworks[i].first, N, iteration);
	      DeleteNetwork(CompletedNetworks[i].first);
	      //height Stats
	      UpdateStats(iteration+1, true);
	  }
      }
  }
  /*if(Solutions.size() - SimpleNetworks > 1) {
      cout<<iteration<<", "<<Net->GetIdx()<<": "<<Solutions.size() - SimpleNetworks<<endl;
      cout<<"  "; for(int i = SimpleNetworks-1; i < Solutions.size(); i++) { if(i != SimpleNetworks-1) cout<<", "; cout<<Solutions[i]->GetIdx();} cout<<endl;
      }*/

  //if(iteration % 10 == 0)  {
  //    cout<<"B"<<iteration<<": "<<Net->Cost()<<": "<<ComputeNetworkCount<<": "<<time(NULL)-begin<<endl;
  //}

  if(NetworksCreated > 1) BranchCount++;
  else ImplicationCount++;
  //width Stats
  UpdateStats(NetworksCreated, false);
  DeleteNetwork(Net);
  if(!BlankNodes.empty()) for(set<Node*>::iterator b = BlankNodes.begin(); b != BlankNodes.end(); b++) delete (*b);

  //if(Solutions.empty())  status<<iteration<<" Return with No Solution"<<endl;
  //cout<<"Return: End ("<<iteration<<")"<<endl; //TEMP

}

int main (int argc, char* argv[]) {
    if(!print) skipToEnd = true;
    string inputStr;
    
    Manager = new Cudd;
    ZERO = Manager->bddZero();
    ONE = Manager->bddOne();
    BDD f;

    int PWR;

    //Initialize Stats
    time_t beginTime = time (NULL);
    ComputeNetworkCount = 0;
    NODECOUNT = 0;
    NETWORKCOUNT = 0;
    CONFLICTCOUNT = 0;
    BranchCount = 0;
    ImplicationCount = 0;
    LeafCount = 0;

    StrucImplCount = 0;
    DuplCoverCount = 0;
    symmetryCount = 0;
    GateSymmCount = 0;

    char choice = 'd';

    //For choice 'c'
    vector<BDD> Functions;
    int FunctionIndex = 0;

    //Create network
    int argct = 1;
    vector<BDD> OutputBdds;
    //int SetUpperBound = INF;
    int SetUpperBound = 11;
    if(all) {
	if(argc < 2) {
	    cout<<"Enter # inputs: ";
	    cin>>inputStr;
	} else {
	    inputStr.assign(argv[argct++]);
	}
	INPUT = stringToint(inputStr);
	for(int i = 0; i < INPUT; i++) {
	    InputBdds.push_back(Manager->bddVar());
	}
	PWR = 1<<INPUT;
	string valueStr;// = "1";
	for(int i = 0; i < PWR-1; i++)
	    valueStr += "0";
	valueStr += "1";
	f = stringTobdd(valueStr);
	OutputBdds.push_back(f);
    } else {
	if(argc < 2) {
	    cout<<"Enter input type (a. blif file, b. integer, c. random file): ";
	    cin>>choice;
	} else {
	    inputStr.assign(argv[argct++]);
	    choice = inputStr[0];
	}
	//cout<<"choice = "<<choice<<endl;
	if(choice == 'a') {
	    if(argc < 3) {
		cout<<"Enter blif file: ";
		cin>>inputStr;
	    } else {
		inputStr.assign(argv[argct++]);
	    }
	    ifstream i_file (inputStr.c_str());
	    ReadBlifFile(i_file, OutputBdds);
	    inputStr = intTostring(INPUT);
	    PWR = 1<<INPUT;
	} else if(choice == 'b') {
	    if(argc < 3) {
		cout<<"Enter Number of Inputs: ";
		cin>>INPUT;
	    } else {
		inputStr.assign(argv[argct++]);
		INPUT = stringToint(inputStr);
	    }
	    for(int i = 0; i < INPUT; i++) {
		InputBdds.push_back(Manager->bddVar());
	    }
	    PWR = 1<<INPUT;
	    cout<<"# Inputs = "<<INPUT<<endl;
	    string ftnStr;
	    int ftn;
	    if(argc < 4) {
		cout<<"Enter Function Index: ";
		cin>>ftn;
	    } else {
		ftnStr.assign(argv[argct++]);
		ftn = stringToint(ftnStr);
	    }
	    f = intTobdd(ftn);
	    OutputBdds.push_back(f);
	    cout<<"Function = "<<ftn<<" = "; PrintFunction(f); cout<<endl;
	} else if(choice == 'c') {
	  if(argc < 3) {
	    cout<<"Enter random file: ";
	    cin>>inputStr;
	  } else {
	    inputStr.assign(argv[argct++]);
	  }
	  ifstream i_file (inputStr.c_str());
	  ReadRandomFile(i_file, Functions);
	  if(Functions.empty()) return 0;
	  cout<<"INPUT = "<<INPUT<<endl;
	  cout<<"Functions = "<<Functions.size()<<endl;
	  PWR = 1<<INPUT;
	  inputStr = intTostring(INPUT);
	  f = Functions[FunctionIndex];
	  FunctionIndex++;
	  OutputBdds.push_back(f);
	}
	if(RelaxVer == 'B' && ((((choice == 'a' || choice == 'c') && argc > 3) || (choice == 'b' && argc > 4)) && isdigit(argv[argct][0]))) {
	    RelaxConst = stringToint(argv[argct]);
	    cout<<"Lower Upper Bound by "<<RelaxConst<<" each time solution is found"<<endl;
	} else if((RelaxVer == 'C' || RelaxVer == 'E') && 
		  ((((choice == 'a' || choice == 'c') && argc > 3) || (choice == 'b' && argc > 4)) && isdigit(argv[argct][0]))) {
	    RelaxConst = stringToint(argv[argct++]);
	    if((((choice == 'a' || choice == 'c') && argc > 4) || (choice == 'b' && argc > 5)) && isdigit(argv[argct][0]))
		RelaxPercent = stringToint(argv[argct]);
	    if(RelaxVer == 'C')
		cout<<"Lower Upper Bound by "<<RelaxPercent<<" every "<<RelaxConst<<" branches"<<endl;
	    else {
		cout<<"Lower Upper Bound by "<<100-RelaxPercent<<"% every "<<RelaxConst<<" branches"<<endl;
		RelaxPercent = RelaxPercent/100;
	    }
	} else if(RelaxVer == 'D' && ((((choice == 'a' || choice == 'c') && argc > 3) || (choice == 'b' && argc > 4)) && isdigit(argv[argct][0]))) {
	    RelaxPercent = float(stringToint(argv[argct]))/100;
	    cout<<"Lower Upper Bound by "<<RelaxPercent<<"% each time solution is found"<<endl;
	} else if((((choice == 'a' || choice == 'c') && argc > 3) || (choice == 'b' && argc > 4)) && isdigit(argv[argct][0])) {
	    SetUpperBound = stringToint(argv[argct]);
	    cout<<"Upper Bound: "<<SetUpperBound<<endl;
	} else if((((choice == 'a' || choice == 'c') && argc > 3) || (choice == 'b' && argc > 4)) && argv[argct][0] >= 'A' && argv[argct][0] <= 'R') {
	    MintermChoice = argv[argct++][0];
	    if((((choice == 'a' || choice == 'c') && argc > 4) || (choice == 'b' && argc > 5)) && argv[argct][0] >= 'A' && argv[argct][0] <= 'F') {
		NetworkOrder = argv[argct++][0];
	    }
	    cout<<"Heuristics: "<<MintermChoice<<"  "<<NetworkOrder<<endl;
	}
    }

    srand(PWR*Number(OutputBdds[0]));
  
    int TotalComputeNetwork = 0;
    int TotalNODECOUNT = 0;
    int TotalNETWORKCOUNT = 0;
    int TotalNonMinimum = 0;
    int TotalBranchCount = 0;
    int TotalImplicationCount = 0;
    int TotalStrucImplCount = 0;
    int TotalLeafCount = 0;

    string fileName = inputStr + "InputSolutions";
    ofstream oFile(fileName.c_str());
    fileName = inputStr + "Status";
    status.open(fileName.c_str());
    fileName = inputStr + "ResultsTable";
    ofstream Results(fileName.c_str());
    ofstream Catalog;
    if(PrintCatalog) {
	fileName = inputStr + "Catalog";
	Catalog.open(fileName.c_str());
    }
    if(RelaxVer > 'A') {
	fileName = inputStr + "Suboptimal";
	Subopt.open(fileName.c_str());
	Subopt<<";;;Initial;;;;;;;1 hour;;;;;;;3 hours;;;;;;;5 hours;;;;;;;10 hours;;;;;;;24 hours;;;;;;;Best;;;;;;;Final"<<endl
	      <<"Complete;Input;Output;"
	      <<"Cost;Gates;Levels;Edges;Time;Calls;Struc Impl;"
	      <<"Cost;Gates;Levels;Edges;Time;Calls;Struc Impl;"
	      <<"Cost;Gates;Levels;Edges;Time;Calls;Struc Impl;"
	      <<"Cost;Gates;Levels;Edges;Time;Calls;Struc Impl;"
	      <<"Cost;Gates;Levels;Edges;Time;Calls;Struc Impl;"
	      <<"Cost;Gates;Levels;Edges;Time;Calls;Struc Impl;"
	      <<"Cost;Gates;Levels;Edges;Time;Calls;Struc Impl;"
	      <<"Cost LB;Cost;Gates;Levels;Edges;Time;Calls;Struc Impl;Branches;Implications;Leaves;SearchTree;"<<endl;
    }

    oFile<<";#;Function;Implementation;# gates;# levels;Time(sec);ending UB;# calls;# branches;# impls;# leaves;min width;max width;avg width;min height;max height;avg height;time/iter"<<endl;
    //Results<<"Input;Output;Complete;Gates;Time(sec);Search Space;Search Width(Avg);Ending UB;Initial Tree?;Final Tree?;Fan-in;Minterm Choice;Struc. Impl.;Symmetry;Gate Symmetry;Duplicate Cover;Network Order;New Bridge;SimpleBridge;Relax;Relax Const.;Relax %;Stop Time"<<endl;
    Results<<"Input;Output;Complete;Cost;Gates;Levels;Edges;"
	//<<"Min Fan-in;Avg Fan-in;Max Fan-in;"
	   <<"Time(sec);"
	   <<"Branches;Implications;Leaves;Ftn Call Count;Search Space;"
	   <<"Height(Min);Height(Avg);Height(Max);"
	   <<"Width > 2;Width(Min);Width(Avg);Width(Max);"
	   <<"Initial Cost;Optimal Node #;Ending UB;Ending Level Bound;Initial Tree?;Final Tree?;"
	   <<"Cost Function;TreeFirst;"
	   <<"Gate Type;ComplementedInputs;"
	   <<"Fan-in Restriction;Fan-out Restriction;Level Restriction;"
	   <<"Minterm Choice;Network Order;"
	   <<"StrucImplCount;SymmtryCount;GateSymmetryCount;DuplCoverCount;"
	   <<"Extended Bridge (All);Extended Bridge (C only);Simple Bridge;"
	   <<"Relax Ver;Relax Const;Relax %;RelaxDone;"
	   <<"Stop Time"<<endl;
    int count = 0;

    while(!OutputBdds.empty()) {
	Results<<InputBdds.size()<<";"<<OutputBdds.size()<<";";
	if(print) {
	    for(int i = 0; i< OutputBdds.size(); i++) {
		cout<<"FUNCTION: "<<Number(OutputBdds[i])<<" = "; 
		PrintFunction(OutputBdds[i]);
		cout<<endl;
	    }
	}
	for(int i = 0; i< OutputBdds.size(); i++) {
	    status<<"FUNCTION: "<<Number(OutputBdds[i])<<" = "; 
	    PrintFunction(OutputBdds[i], status);
	    status<<endl;
	}

    
	STOP = false;
	SetTreeFirst = false; //&&
	if(RelaxVer > 'A') TreeFirst = false;    //&&
	if(RelaxVer == 'E') RelaxDone = 1;
	count++;

	InitialTreeStructure = false;

	minWidth = minHeight = INF;
	maxWidth = ctWidth = maxHeight = ctHeight = 0;
	avgWidth = avgHeight = 0;
	More2Width = 0;
	ExtendedBridgeCount = 0;
	SimpleBridgeCount = 0;

	//Suboptmal Details
	if(RelaxVer > 'A') {
	    SuboptDetails.clear();
	    SuboptDetails.resize(7);
	}

	//Set Bounds
	UpperBound = SetUpperBound;
	LevelBound = INF;

	begin = time (NULL);

	int NumberInitialNetworks = 1;
	if(GateType == 3) for(int i = 0; i < OutputBdds.size(); i++) NumberInitialNetworks *= 3;
	if(GateType == 4) for(int i = 0; i < OutputBdds.size(); i++) NumberInitialNetworks *= 2;
	if(GateType == 5) for(int i = 0; i < OutputBdds.size(); i++) NumberInitialNetworks *= 2;
	for(int n = 0; n < NumberInitialNetworks; n++) {
	    //Initialize Network
	    Network* Original = new Network;
	    for(int i = 0; i < InputBdds.size(); i++) Original->InsertInput(InputBdds[i]);
	    //Original->InsertInput(!InputBdds[InputBdds.size()-1]);
	    int ct = n;
	    for(int i = 0; i < OutputBdds.size(); i++)
		if(GateType == 1 || GateType == 2) Original->InsertOutput(OutputBdds[i]);
		else if(GateType == 3) {
		    if(ct % 3 == 0) Original->InsertOutput(OutputBdds[i], 'A');
		    else if(ct % 3 == 1) Original->InsertOutput(OutputBdds[i], 'O'); 
		    else if(ct % 3 == 2) Original->InsertOutput(OutputBdds[i], 'N'); 
		    ct /= 3;
		} else if(GateType == 4) {
		    if(MoreOnes(OutputBdds[0])) {
			if(ct % 2 == 1) Original->InsertOutput(OutputBdds[i], 'A');
			else if(ct % 2 == 0) Original->InsertOutput(OutputBdds[i], 'O');
		    } else {
			if(ct % 2 == 0) Original->InsertOutput(OutputBdds[i], 'A');
			else if(ct % 2 == 1) Original->InsertOutput(OutputBdds[i], 'O');
		    }
		    ct /= 2;
		} else if(GateType == 5) {
		    if(MoreOnes(OutputBdds[0])) {
			if(ct % 2 == 1) Original->InsertOutput(OutputBdds[i], 'D');
			else if(ct % 2 == 0) Original->InsertOutput(OutputBdds[i], 'R');
		    } else {
			if(ct % 2 == 0) Original->InsertOutput(OutputBdds[i], 'R');
			else if(ct % 2 == 1) Original->InsertOutput(OutputBdds[i], 'D');
		    }
		    ct /= 2;
		}
	    if(symmetry) Original->ComputeSymmetries();
	    Original->SetCount(INF);
	    Original->Initialization();
	    if(print) cout<<"Initialization"<<endl; 
	    if(trace) status<<"Initialization"<<endl; 
	    if(trace) Original->PrintPicture(status);
	    if(!skipToEnd) DoNext(Original, NULL, -1);
	    ComputeNetwork(Original, 1);

	    //&&
	    if(STOP && SetTreeFirst) {
		cout<<"START OVER"<<endl;
		status<<"START OVER"<<endl;
		UpperBound = INF;
		STOP = false;  SetTreeFirst = false;
		TreeFirst = true;
		ComputeNetworkCount = 0; NODECOUNT = 0;	NETWORKCOUNT = 0; BranchCount = 0; ImplicationCount = 0; LeafCount = 0; StrucImplCount = 0;
		DuplCoverCount = 0; symmetryCount = 0; GateSymmCount = 0;
		minWidth = minHeight = INF; maxWidth = ctWidth = maxHeight = ctHeight = 0; avgWidth = avgHeight = 0; More2Width = 0;
		ExtendedBridgeCount = 0; SimpleBridgeCount = 0;	
		Network* Second = new Network;
		for(int i = 0; i < InputBdds.size(); i++) Second->InsertInput(InputBdds[i]);
		int ct = n;
		for(int i = 0; i < OutputBdds.size(); i++)
		    if(GateType == 1 || GateType == 2) Second->InsertOutput(OutputBdds[i]);
		    else if(GateType == 3) {
			if(ct % 3 == 0) Second->InsertOutput(OutputBdds[i], 'A'); 
			else if(ct % 3 == 1) Second->InsertOutput(OutputBdds[i], 'O'); 
			else if(ct % 3 == 2) Second->InsertOutput(OutputBdds[i], 'N'); 
			ct /= 3;
		    } else if(GateType == 4) {
			if(MoreOnes(OutputBdds[0])) {
			    if(ct % 2 == 1) Second->InsertOutput(OutputBdds[i], 'A');
			    else if(ct % 2 == 0) Second->InsertOutput(OutputBdds[i], 'O');
			} else {
			    if(ct % 2 == 0) Second->InsertOutput(OutputBdds[i], 'A');
			    else if(ct % 2 == 1) Second->InsertOutput(OutputBdds[i], 'O');
			}
			ct /= 2;
		    } else if(GateType == 5) {
			if(MoreOnes(OutputBdds[0])) {
			    if(ct % 2 == 1) Second->InsertOutput(OutputBdds[i], 'D');
			    else if(ct % 2 == 0) Second->InsertOutput(OutputBdds[i], 'R');
			} else {
			    if(ct % 2 == 0) Second->InsertOutput(OutputBdds[i], 'R');
			    else if(ct % 2 == 1) Second->InsertOutput(OutputBdds[i], 'D');
			}
			ct /= 2;
		    }
		if(symmetry) Second->ComputeSymmetries();
		Second->SetCount(INF);
		Second->Initialization();
		if(!skipToEnd) DoNext(Second, NULL, -1);
		ComputeNetwork(Second, 1);
		
	    }
	}
	
	//&&
	
	if(FirstNetwork) {
	    UpperBound++;
	    STOP = false;
	    int NumberInitialNetworks = 1;
	    if(GateType == 3) for(int i = 0; i < OutputBdds.size(); i++) NumberInitialNetworks *= 3;
	    if(GateType == 4) for(int i = 0; i < OutputBdds.size(); i++) NumberInitialNetworks *= 2;
	    if(GateType == 5) for(int i = 0; i < OutputBdds.size(); i++) NumberInitialNetworks *= 2;
	    for(int n = 0; n < NumberInitialNetworks; n++) {
		Network* Second = new Network;
		for(int i = 0; i < InputBdds.size(); i++) Second->InsertInput(InputBdds[i]);
		int ct = n;
		for(int i = 0; i < OutputBdds.size(); i++)
		    if(GateType == 1 || GateType == 2) Second->InsertOutput(OutputBdds[i]);
		    else if(GateType == 3) {
			if(ct % 3 == 0) Second->InsertOutput(OutputBdds[i], 'A');
			else if(ct % 3 == 1) Second->InsertOutput(OutputBdds[i], 'O');
			else if(ct % 3 == 2) Second->InsertOutput(OutputBdds[i], 'N');
			ct /= 3;
		    } else if(GateType == 4) {
			if(ct % 2 == 0) Second->InsertOutput(OutputBdds[i], 'A');
			else if(ct % 2 == 1) Second->InsertOutput(OutputBdds[i], 'O');
			ct /= 2;
		    } else if(GateType == 5) {
			if(ct % 2 == 0) Second->InsertOutput(OutputBdds[i], 'D');
			else if(ct % 2 == 1) Second->InsertOutput(OutputBdds[i], 'R');
			ct /= 2;
		    }
		if(symmetry) Second->ComputeSymmetries();
		Second->SetCount(INF);
		Second->Initialization();
		if(!skipToEnd) DoNext(Second, NULL, -1);
		ComputeNetwork(Second, 1);
	    }
	}
	//Solutions
	time_t end = time (NULL);
	for(int i = 0; i < Solutions.size(); i++) {
	    time_t tempTime = time (NULL);
	    //InputSolutions
	    if(STOP) oFile<<"*"; oFile<<";";
	    Solutions[i]->PrintSimpleSolution(true, oFile);
	    oFile<<end-begin<<";"<<UpperBound<<";"<<ComputeNetworkCount<<";"<<BranchCount<<";"<<ImplicationCount<<";"<<LeafCount;
	    oFile<<";"<<More2Width<<";"<<minWidth<<";"<<maxWidth<<";"<<avgWidth<<";"<<minHeight<<";"<<maxHeight<<";"<<avgHeight<<";"
		 <<(float) (end-begin)/ComputeNetworkCount;
	    oFile<<endl; 
	    //Cout
	    if(print) {cout<<endl<<"Solution: "<<endl; Solutions[i]->PrintPicture();}
	    //Status
	    Solutions[i]->PrintPicture(status);
	    //ResultsTable
	    if(i != 0) Results<<";;";
	    if(STOP) 
		if(FirstNetwork) Results<<"First";
		else Results<<"No"; 
	    else Results<<"Yes";
	    Results<<";"<<Solutions[i]->Cost()<<";"<<Solutions[i]->Gates()<<";"<<Solutions[i]->Level()<<";"<<Solutions[i]->Edges()<<";";
	    //<<Solutions[i]->PrintFanin()<<";"
	    Results<<end-begin<<";"
		   <<BranchCount<<";"<<ImplicationCount<<";"<<LeafCount<<";"<<ComputeNetworkCount<<";"
		   <<BranchCount+ImplicationCount-StrucImplCount+LeafCount<<";"
		   <<minHeight<<";"<<avgHeight<<";"<<maxHeight<<";"
		   <<More2Width<<";"<<minWidth<<";"<<avgWidth<<";"<<maxWidth<<";"
		   <<InitialCost<<";"<<OptimalNodeNum<<";"<<UpperBound<<";";
	    if(LevelBoundCheck) Results<<LevelBound<<";"; else Results<<"-;"; 
	    Results<<InitialTreeStructure<<";"<<Solutions[i]->TreeStructure()<<";"
		   <<gateRank<<"*gates+"<<edgeRank<<"*edges+"<<levelRank<<"*levels"<<";"
		   <<TreeFirst<<";"<<GateType<<";"<<ComplementedInputs<<";";
	    if(FaninBound == INF) Results<<"-"; else Results<<FaninBound; Results<<";";
	    if(FanoutRestrict) Results<<FanoutRestriction; else Results<<"-"; Results<<";";
	    if(LevelRestrict) Results<<LevelRestriction; else Results<<"-"; Results<<";";
	    if(GateType == 4) Results<<"-;-;"; else Results<<MintermChoice<<";"<<NetworkOrder<<";";
	    if(StrucImpl) Results<<StrucImplCount; else Results<<"-"; Results<<";";
	    if(symmetry) Results<<symmetryCount; else Results<<"-"; Results<<";";
	    if(GateSymm) Results<<GateSymmCount; else Results<<"-"; Results<<";";
	    if(DuplCover) Results<<DuplCoverCount; else Results<<"-"; Results<<";";
	    if(ExtendedBridgeAll) Results<<ExtendedBridgeCount; else Results<<"-"; Results<<";";
	    if(ExtendedBridgeC) Results<<ExtendedBridgeCount; else Results<<"-"; Results<<";";
	    if(SimpleBridge) Results<<SimpleBridgeCount; else Results<<"-"; Results<<";";
	    Results<<RelaxVer<<";"; 
	    if(RelaxVer == 'B' || RelaxVer == 'C' || RelaxVer == 'E' || RelaxVer == 'F') Results<<RelaxConst<<";"; else Results<<"-;";
	    if(RelaxVer == 'C' || RelaxVer == 'D' || RelaxVer == 'E') Results<<RelaxPercent<<";"; else Results<<"-;";
	    Results<<RelaxDone<<";";
	    Results<<StopTime<<endl;
	    //REMOVE
	    //cout<<Solutions[i]->GetNumber()<<":";
	    //if(Solutions[i]->OrFirst()) cout<<"OR";
	    //else if(Solutions[i]->AndFirst()) cout<<"AND";
	    //cout<<":"<<InitialCost<<":"<<Solutions[i]->Cost()<<endl;
	}
	if(RelaxVer > 'A') {
	    //"Input;Output;Cost;Gates;Levels;Edges;Time;Branches;Implications;Leaves;Search Tree;"
	    Subopt<<!STOP<<";"<<InputBdds.size()<<";"<<OutputBdds.size()<<";";
	    if(!Solutions.empty()) {
		for(int j = 0; j < SuboptDetails.size(); j++) {
		    bool printDetails = false;
		    if(j == 0 || SuboptDetails[j][0] != SuboptDetails[j-1][0])
			printDetails = true;
		    for(int k = 0; k < SuboptDetails[j].size(); k++) {
			if(printDetails)
			    Subopt<<SuboptDetails[j][k]<<";";
			else
			    Subopt<<"-;";
		    }
		}
		Network* SNet = Solutions[0];
		float CostLB; if(STOP) CostLB = 0; else CostLB = UpperBound;
		Subopt<<CostLB<<";"<<SNet->Cost()<<";"<<SNet->Gates()<<";"<<SNet->Level()<<";"<<SNet->Edges()<<";";
	    } else {
		Subopt<<";;;;;;;"
		      <<";;;;;;;"
		      <<";;;;;;;"
		      <<";;;;;;;"
		      <<";;;;;;;"
		      <<";;;;;;;"
		      <<";;;;;;;"
		      <<";;;;;";
	    }
	    Subopt<<end-begin<<";"
		  <<ComputeNetworkCount<<";"<<StrucImplCount<<";"
		  <<BranchCount<<";"<<ImplicationCount<<";"<<LeafCount<<";"
		  <<BranchCount+ImplicationCount-StrucImplCount+LeafCount<<endl;
	}
	if(PrintCatalog && !Solutions.empty()) {
	    for(int j = 0; j < Solutions.size(); j++) {
		for(int i = 0; i < OutputBdds.size(); i++) {
		    if(i != 0) Catalog<<";";
		    PrintFunction(OutputBdds[i], Catalog);
		    Catalog<<";"<<Number(OutputBdds[i]);
		}
		Catalog<<";";
		Network* SNet = Solutions[j];
		SNet->PrintCatalog(Catalog);
		Catalog<<endl;
	    }
	}
	if(Solutions.empty()) {
	    Results<<"No;;;;;"
		//<<";;;"  //Fan-in
		   <<end-begin<<";"
		   <<BranchCount<<";"<<ImplicationCount<<";"<<LeafCount<<";"<<ComputeNetworkCount<<";"
		   <<BranchCount+ImplicationCount-StrucImplCount+LeafCount<<";"
		   <<minHeight<<";"<<avgHeight<<";"<<maxHeight<<";"
		   <<More2Width<<";"<<minWidth<<";"<<avgWidth<<";"<<maxWidth<<";"
		   <<InitialCost<<";"<<OptimalNodeNum<<";"<<UpperBound<<";";
	    if(LevelBoundCheck) Results<<LevelBound<<";"; else Results<<"-;"; 
	    Results<<";;"
		   <<gateRank<<"*gates+"<<edgeRank<<"*edges+"<<levelRank<<"*levels"<<";"<<TreeFirst<<";"
		   <<GateType<<";"<<ComplementedInputs<<";";
	    if(FaninBound == INF) Results<<"-"; else Results<<FaninBound; Results<<";";
	    if(FanoutRestrict) Results<<FanoutRestriction; else Results<<"-"; Results<<";";
	    if(LevelRestrict) Results<<LevelRestriction; else Results<<"-"; Results<<";";
	    if(GateType == 4) Results<<"-;-;"; else Results<<MintermChoice<<";"<<NetworkOrder<<";";
	    if(StrucImpl) Results<<StrucImplCount; else Results<<"-"; Results<<";";
	    if(symmetry) Results<<symmetryCount; else Results<<"-"; Results<<";";
	    if(GateSymm) Results<<GateSymmCount; else Results<<"-"; Results<<";";
	    if(DuplCover) Results<<DuplCoverCount; else Results<<"-"; Results<<";";
	    if(ExtendedBridgeAll) Results<<ExtendedBridgeCount; else Results<<"-"; Results<<";";
	    if(ExtendedBridgeC) Results<<ExtendedBridgeCount; else Results<<"-"; Results<<";";
	    if(SimpleBridge) Results<<SimpleBridgeCount; else Results<<"-"; Results<<";";
	    Results<<RelaxVer<<";"; 
	    if(RelaxVer == 'B' || RelaxVer == 'C' || RelaxVer == 'D') Results<<RelaxConst<<";"; else Results<<"-;";
	    if(RelaxVer == 'D' || RelaxVer == 'E') Results<<RelaxPercent<<";"; else Results<<"-;";
	    Results<<RelaxDone<<";";
	    Results<<StopTime<<endl;
	}
	
	for(int i = 0; i < Solutions.size(); i++)
	    delete Solutions[i];
	Solutions.clear();
	
	int hrs = 0; int min = 0; int sec = end-begin;
	if(sec/60 > 0) { min = sec/60; sec -= min*60; }
	if(min/60 > 0) { hrs = min/60; min -= hrs*60; }
	if(!print) {
	    status<<"time = "; 
	    if(hrs > 0) status<<hrs<<" hrs "; 
	    if(min > 0) status<<min<<" mins "; 
	    if(sec > 0 || (hrs == 0 && min == 0)) status<<sec<<" secs "; 
	    status<<endl;
	    }
	status<<"ComputeNetwork was called "<<ComputeNetworkCount<<" times"<<endl;
	status<<"# Branch Choices = "<<BranchCount<<endl;
	status<<"# Implications = "<<ImplicationCount<<endl;
	status<<"# Structural Implications = "<<StrucImplCount<<endl;
	status<<"# SearchSpace Leaves = "<<LeafCount<<endl;
	status<<"# nodes = "<<NODECOUNT<<endl;
	status<<"# networks = "<<NETWORKCOUNT<<endl;
	status<<"# conflicts = "<<CONFLICTCOUNT<<endl;
	status<<endl;

	if(!STOP) {
	  TotalComputeNetwork += ComputeNetworkCount;
	  TotalNODECOUNT += NODECOUNT;
	  TotalNETWORKCOUNT += NETWORKCOUNT;
	  TotalBranchCount += BranchCount;
	  TotalImplicationCount += ImplicationCount;
	  TotalStrucImplCount += StrucImplCount;
	  TotalLeafCount += LeafCount;
	}
	ComputeNetworkCount = 0;
	NODECOUNT = 0;
	NETWORKCOUNT = 0;
	BranchCount = 0;
	ImplicationCount = 0;
	LeafCount = 0;

	StrucImplCount = 0;
	DuplCoverCount = 0;
	symmetryCount = 0;
	GateSymmCount = 0;
	
	for(map<int,vector<Network*> >::iterator i = AllNetworks.begin(); i != AllNetworks.end(); i++) {
	    vector<Network*> SomeNets = (*i).second;
	    for(int j = 0; j < SomeNets.size(); j++) {
		delete SomeNets[j];
	    }
	}
	AllNetworks.clear();
	
	OutputBdds.clear();
	if(!all && choice == 'c') {
	    if(FunctionIndex >= Functions.size()) OutputBdds.clear();
	    else {
		f = Functions[FunctionIndex];
		FunctionIndex++;
		OutputBdds.clear();
		OutputBdds.push_back(f);
	    }
	} else if(all) {    
	    if(count == 100)
		OutputBdds.clear();
	    else {
		do {
		    for(int i = 1; i <= PWR; i++) {
			BDD M = MakeMinterm(InputBdds, PWR-i);
			if(f.Constrain(M) == ONE)
			    f *= !M;
			else { 
			    f += M;
			    break;
			}
		    }
		} while((!Dependents(f) || !NewFunction(f)) && f != ONE);
		if(f != ONE) {
		    OutputBdds.clear();
		    OutputBdds.push_back(f);
		} else OutputBdds.clear();
	    }
	} else if(!all && choice == 'a') {
	    OutputBdds.clear();
	}  else OutputBdds.clear();
    }
    
    time_t endTime = time (NULL);
    int hrs = 0; int min = 0; int sec = endTime-beginTime;
    if(sec/60 > 0) { min = sec/60; sec -= min*60; }
    if(min/60 > 0) { hrs = min/60; min -= hrs*60; }
    if(!print) {
	status<<"This program took "; 
	if(hrs > 0) status<<hrs<<" hrs "; 
	if(min > 0) status<<min<<" mins "; 
	if(sec > 0 || (hrs == 0 && min == 0)) status<<sec<<" secs "; 
	status<<"to complete."<<endl;
    }
    status<<"This program made "<<TotalComputeNetwork<<" calls to ComputeNetwork."<<endl;
    status<<"This program has "<<TotalBranchCount<<" branch points in the search space."<<endl;
    status<<"This program has "<<TotalImplicationCount<<" implication points in the search space."<<endl;
    status<<"This program has "<<TotalStrucImplCount<<" structural implication points in the search space."<<endl;
    status<<"This program has "<<TotalLeafCount<<" leaf points in the search space."<<endl;
    status<<"This program used "<<TotalNODECOUNT<<" nodes."<<endl;
    status<<"This program used "<<TotalNETWORKCOUNT<<" networks."<<endl;
    
    if(!print) {
	oFile<<"This program took "; 
	if(hrs > 0) oFile<<hrs<<" hrs "; 
	if(min > 0) oFile<<min<<" mins "; 
	if(sec > 0 || (hrs == 0 && min == 0)) oFile<<sec<<" secs "; 
	oFile<<"to complete."<<endl;
    }
    oFile<<"This program made "<<TotalComputeNetwork<<" calls to ComputeNetwork."<<endl;
    oFile<<"This program has "<<TotalBranchCount<<" branch points in the search space."<<endl;
    oFile<<"This program has "<<TotalImplicationCount<<" implication points in the search space."<<endl;
    oFile<<"This program has "<<TotalStrucImplCount<<" structural implication points in the search space."<<endl;
    oFile<<"This program has "<<TotalLeafCount<<" leaf points in the search space."<<endl;
    oFile<<"This program used "<<TotalNODECOUNT<<" nodes."<<endl;
    oFile<<"This program used "<<TotalNETWORKCOUNT<<" networks."<<endl;
    status<<"Fan-in: "<<FaninBound<<"; MintermChoice: "<<MintermChoice<<"; Structrual Implication: "<<StrucImpl<<"; Symmetry: "<<symmetry
	  <<"; Gate Symmetry: "<<GateSymm<<"; Duplicate Cover: "<<DuplCover<<"; Network Order: "<<NetworkOrder<<"; Extended Bridge: "<<ExtendedBridgeCount
	  <<"; Simple Bridge: "<<SimpleBridgeCount<<"; Relax Version : "<<RelaxVer<<"; Relax Constant: "<<RelaxConst<<"; Relax %: "<<RelaxPercent
	  <<"; Stop Time: "<<StopTime<<endl;
    oFile<<"Fan-in: "<<FaninBound<<"; MintermChoice: "<<MintermChoice<<"; Structrual Implication: "<<StrucImpl<<"; Symmetry: "<<symmetry
	 <<"; Gate Symmetry: "<<GateSymm<<"; Duplicate Cover: "<<DuplCover<<"; Network Order: "<<NetworkOrder<<"; Extended Bridge: "<<ExtendedBridgeCount
	 <<"; Simple Bridge: "<<SimpleBridgeCount<<"; Relax Version : "<<RelaxVer<<"; Relax Constant: "<<RelaxConst<<"; Relax %: "<<RelaxPercent
	 <<"; Stop Time: "<<StopTime<<endl;
    Reps.clear();
    RepSize.clear();
    RepInput.clear();
}


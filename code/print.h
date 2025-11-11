#include "node.h"
#include <iomanip>
//Do Next Globas                                                                              
int skipToIteration = -1;
int skipToNode = -1;
bool skipToSolution = false;
bool noPrint = false;

void DoNext(Network* Net, Node* N, int iteration) {
  string mystr;
  if((!skipToEnd) &&
     (skipToIteration == -1 || skipToIteration == iteration) &&
     (skipToNode == -1 || (N != NULL && skipToNode == N->idx)) &&
     (!skipToSolution || N == NULL)) {
    skipToNode = -1;
    skipToIteration = -1;
    skipToSolution = false;
    noPrint = false;
    print = true;
    if(Net != NULL) Net->PrintPicture();
    cout<<" > ";
    getline(cin,mystr);
    cout<<"Your choice is "<<mystr<<"."<<endl;
    if(mystr.find(" -print") != string::npos) {
      int pos = mystr.find(" -print");
      mystr = mystr.substr(0,pos);
      noPrint = true;
      print = false;
    }
    if(mystr.find("iter") != string::npos) {
      string sub = mystr.substr(5);
      skipToIteration = stringToint(sub);
    } else if(mystr.find("sol") != string::npos) {
      skipToSolution = true;
    } else if(mystr.find("node") != string::npos) {
      string sub = mystr.substr(5);
      skipToNode = stringToint(sub);
    } else if(mystr.find("end") != string::npos) {
      skipToEnd = true;
    } else if(mystr.find("print") != string::npos) {
      print = false;
    }
  } else {
    if(!noPrint && print) {
	if(Net != NULL) Net->PrintPicture();
	cout<<endl;
    }
  }
}
void ErrorPrint(string msg, Network* Net, Node* N, int iteration) {
    if(print) cout<<"  "<<msg<<endl;
    if(trace) status<<"  "<<msg<<endl;                                                        
    if(trace && Net != NULL) Net->PrintPicture(status);
    if(!skipToEnd) DoNext(Net, N, iteration);
}


void PrintShortDetails(int iteration, Node* N, BDD ToCover) {
    if(print) {
	cout<<"Iteration "<<iteration<<endl; 
	cout<<"  "<<N<<": ";
	PrintInterval(N->ON, N->OFF);
	cout<<endl;
	cout<<"  ToCover("<<N<<") = "; PrintFunction(ToCover); cout<<endl;
    }
    if(trace) { 
	status<<"Iteration "<<iteration<<endl; 
	status<<"  "<<N<<": ";
	PrintInterval(N->ON, N->OFF, status);
	status<<endl;
	status<<"  ToCover("<<N<<") = "; PrintFunction(ToCover,status); status<<endl;
    }
}
void PrintBranch(Node* N, Node* C) {
    if(print) cout<<"Cover "<<N<<" using "<<C<<endl;
    if(trace) status<<"Cover "<<N<<" using "<<C<<endl;
}

void PrintSelectedMinterm(BDD SelectedMinterm) {
    if(print) { cout<<"  SelectedMinterm set to "; PrintFunction(SelectedMinterm); cout<<endl; }
    if(trace) { status<<"  SelectedMinterm set to "; PrintFunction(SelectedMinterm, status); status<<endl; }
}
void PrintMayCoverMinterm(Node* C, BDD SelectedMinterm) {
    if(print) { cout<<"  Pick Minterm from MayCover("<<C<<") = "; PrintFunction(SelectedMinterm); cout<<endl; }
    if(trace) { status<<"  Pick Minterm from MayCover("<<C<<") = "; PrintFunction(SelectedMinterm,status); status<<endl; }
}
void PrintIteration(int iteration) {
    if(print)
	cout<<"Iteration "<<iteration<<"."<<endl;
    if(trace)
	status<<"Iteration "<<iteration<<"."<<endl;
}

void PrintToCover(Node* N, BDD ToCover) {
  if(print) {
      cout<<"To Cover"<<endl;
      cout<<"  "<<N<<": ";
      PrintInterval(N->ON, N->OFF);
      cout<<endl;
      cout<<"  ToCover("<<N<<") = "; PrintFunction(ToCover); cout<<endl;
  }
  if(trace) {
      status<<"  "<<N<<": ";
      PrintInterval(N->ON, N->OFF, status);
      status<<endl;
      status<<"  ToCover("<<N<<") = "; PrintFunction(ToCover,status); status<<endl;
  }
}
void PrintConnectible(BDD ToCover, vector<Node*> Connectible) {
    if(print) {
	if(INPUT < 6) {
	    int ftnwidth = (1<<INPUT) + (1<<(InputBdds.size()/2)) - 1;
	    int width = ftnwidth;
	    if(width < 10) width = 10;
	    cout<<" "; for(int j = 0; j < 2*width+18; j++) cout<<"-"; cout<<endl;
	    cout<<"  Connectible | "<<setw(width)<<"Must Cover"<<" |"<<" Can Cover"<<endl;
	    for(int i = 0; i < Connectible.size(); i++) {
		cout<<"  "<<Connectible[i]<<setw(11)<<" | ";
		PrintFunction(ToCover * Connectible[i]->OFF);
		if(ToCover * Connectible[i]->OFF == ZERO)
		    for(int j = 0; j < width - 1; j++) cout<<" ";
		else
		    for(int j = 0; j < width-ftnwidth; j++) cout<<" ";
		cout<<" | ";
		PrintFunction(ToCover * !Connectible[i]->OFF * !Connectible[i]->ON);
		cout<<endl;
	    }
	    cout<<" "; for(int j = 0; j < 2*width+18; j++) cout<<"-"; cout<<endl;
	} else {
	    cout<<"Connectible: ";
	    for(int i = 0; i < Connectible.size(); i++) {
		cout<<" "<<Connectible[i];
	    }
	    cout<<endl;
	}
    }
    if(trace) {
	int ftnwidth = (int) (1.5*(1<<INPUT))-1;
	int width = ftnwidth;
	if(width < 10) width = 10;
	status<<" "; for(int j = 0; j < 2*width+18; j++) status<<"-"; status<<endl;
	status<<"  Connections | ";
	status<<setw(width)<<"Must Cover |";
	status<<setw(width)<<" Can Cover";
	status<<endl;
	for(int i = 0; i < Connectible.size(); i++) {
	    status<<"  "<<Connectible[i]<<setw(11)<<" | ";
	    PrintFunction(ToCover * Connectible[i]->OFF, status);
	    if(ToCover * Connectible[i]->OFF == ZERO)
		for(int j = 0; j < width - 1; j++) status<<" ";
	    else
		for(int j = 0; j < width-ftnwidth; j++) status<<" ";
	    status<<" | ";
	    PrintFunction(ToCover * !Connectible[i]->OFF * !Connectible[i]->ON, status);
	    status<<endl;
	}
	status<<" "; for(int j = 0; j < 2*width+18; j++) status<<"-"; status<<endl;
    }
}
void PrintIterationSolution(int iteration, float UpperBound, int time, Network* Net) {
    if(print) {
	cout<<"  SOLUTION: Upper Bound = "<<UpperBound<<", Network Cost = "<<Net->Cost()<<", "<<time<<"s"<<endl;
    }
    status<<"Iteration "<<iteration<<"."<<endl;
    status<<"  SOLUTION"; 
    if(GateType == 4) { if(Net->OrFirst()) status<<"(OR)"; else if(Net->AndFirst()) status<<"(AND)"; }
    status<<": Upper Bound = "<<UpperBound<<", "; 
    if(LevelBoundCheck) status<<"Level Bound = "<<LevelBound<<", "; 
    status<<"Network Cost = "<<Net->Cost()<<", "<<time<<"s"<<endl;
    if(trace) Net->PrintPicture(status);
    if(!skipToEnd) DoNext(Net, NULL, iteration);
}
void PrintStack(stack<pair<Node*,BDD> > Stack) {
    if(print || trace) {
	bool firstTime = true;
	if(print) cout<<"Stack: ";
	if(trace) status<<"Stack: ";
	if(Stack.empty()) {
	    if(print) cout<<endl;
	    if(trace) status<<endl;
	}
	while(!Stack.empty()) {
	    if(!firstTime) {
		if(print) cout<<"       ";
		if(trace) status<<"       ";
	    }
	    firstTime = false;
	    if(print) {cout<<"("<<Stack.top().first<<", "; PrintFunction(Stack.top().second); cout<<")"<<endl; }
	    if(trace) {status<<"("<<Stack.top().first<<", "; PrintFunction(Stack.top().second, status); status<<")"<<endl;}
	    Stack.pop();
	}
    }
}
void PrintEither(Node* Left, Node* Right, BDD EitherCover) {
    if(print) { cout<<"  Either "<<Left<<" or "<<Right<<" can cover the minterms "; PrintFunction(EitherCover); cout<<endl; }
    if(trace) { status<<"  Either "<<Left<<" or "<<Right<<" can cover the minterms "; PrintFunction(EitherCover,status); status<<endl; }
}

void PrintFullEither(int iteration, Node* N, Node* Left, Node* Right, BDD ToCover, BDD EitherCover) {
    if(print) {
	cout<<"Iteration "<<iteration<<endl; 
	cout<<"  "<<N<<": ";
	PrintInterval(N->ON, N->OFF);
	cout<<endl;
	cout<<"  ToCover("<<N<<") = "; PrintFunction(ToCover); cout<<endl;
	cout<<"  *BRANCH*"<<endl<<"  Cover "<<N<<" with "<<Left<<" and "<<Right<<endl;
	cout<<"  Either "<<Left<<" or "<<Right<<" can cover the minterms "; PrintFunction(EitherCover); cout<<endl;
    }
    if(trace) {
	status<<"Iteration "<<iteration<<endl; 
	status<<"  "<<N<<": ";
	PrintInterval(N->ON, N->OFF, status);
	status<<endl;
	status<<"  ToCover("<<N<<") = "; PrintFunction(ToCover,status); status<<endl;
	status<<"  *BRANCH*"<<endl<<"  Cover "<<N<<" with "<<Left<<" and "<<Right<<endl;
	status<<"  Either "<<Left<<" or "<<Right<<" can cover the minterms "; PrintFunction(EitherCover,status); status<<endl;
    }
}
void PrintCovers(Node* Left, Node* Right, BDD LeftCover, BDD RightCover) {
    if(print) {
	cout<<"  "<<Left<<" covers "; PrintFunction(LeftCover); cout<<endl;
	cout<<"  "<<Right<<" covers "; PrintFunction(RightCover); cout<<endl;
    }
    if(trace) {
	status<<"  "<<Left<<" covers "; PrintFunction(LeftCover,status); status<<endl;
	status<<"  "<<Right<<" covers "; PrintFunction(RightCover,status); status<<endl;
    }
}
void PrintEnd(Network* NewNetwork, Node* NewN, int iteration) {
  if(trace) NewNetwork->PrintPicture(status);
  if(!skipToEnd) DoNext(NewNetwork, NewN, iteration);
}

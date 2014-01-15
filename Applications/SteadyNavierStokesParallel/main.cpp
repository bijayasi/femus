#include "ElemType.hpp"
#include "NonLinearMultiLevelProblem.hpp"
#include "NumericVector.hpp"
#include "PetscVector.hpp"
#include "LinearSolver.hpp"
#include "Fluid.hpp"
#include "Parameter.hpp"
#include "FemTTUInit.hpp"
using std::cout;
using std::endl;
   

int AssembleMatrixResNS(NonLinearMultiLevelProblem &nl_ml_prob, unsigned level, const unsigned &gridn);

double InitVariables(const double &x, const double &y, const double &z,const char name[]);

bool SetBoundaryCondition(const double &x, const double &y, const double &z,const char name[], 
			  double &value, const int FaceName, const double time);

bool SetRefinementFlag(const double &x, const double &y, const double &z, const int &ElemGroupNumber,const int &level);

int main(int argc,char **args) {
  
  /// Init Petsc-MPI communicator
  FemTTUInit mpinit(argc,args,MPI_COMM_WORLD);
  
  /// INIT MESH =================================  
  
  unsigned short nm,nr;
  nm=1;
  std::cout<<"MULTIGRID levels: "<< nm << endl;

  nr=1;
  std::cout<<"MAX_REFINEMENT levels: " << nr << endl<< endl;
  
  int tmp=nm;  nm+=nr;  nr=tmp;
  
  char *infile = new char [50];
 
  sprintf(infile,"./input/nsbenc.neu");
  
  //Adimensional quantity (Lref,Uref)
  double Lref = 1.;
  double Uref = 1.;
  Parameter parameter(Lref,Uref);
  
  // Generate fluid Object (Adimensional quantities,viscosity,density,fluid-model)
  Fluid fluid(parameter,0.001,1.,"Newtonian");
  
  //Steadystate NonLinearMultiLevelProblem  
  NonLinearMultiLevelProblem nl_ml_prob(nm,nr,infile,"seventh",Lref,SetRefinementFlag);
  
  /// END MESH =================================  
  
  ///Start System Variables;===========================
  //Focus here is on VARIABLES first, rather than on Equations
 
  // add fluid material
  nl_ml_prob.Add_Fluid(&fluid);
  
  // generate solution vector
  nl_ml_prob.AddSolutionVector("U","biquadratic");
  nl_ml_prob.AddSolutionVector("V","biquadratic");
  // the pressure variable should be the last for the Schur decomposition
  nl_ml_prob.AddSolutionVector("P","disc_linear");
  // nl_ml_prob.AddSolutionVector("P","linear");
  nl_ml_prob.AssociatePropertyToSolution("P","Pressure");
  //nl_ml_prob.AssociatePropertyToSolution("P","Default");
  
  //Initialize (update Init(...) function)
  nl_ml_prob.AttachInitVariableFunction(InitVariables);
  nl_ml_prob.Initialize("All");
  
  //Set Boundary (update Dirichlet(...) function)
  nl_ml_prob.AttachSetBoundaryConditionFunction(SetBoundaryCondition);
  nl_ml_prob.GenerateBdc("U");
  nl_ml_prob.GenerateBdc("V");
  nl_ml_prob.GenerateBdc("P");
  
  ///End System Variables; ==============================

  // START EQUATIONS =================================  
  
  /// Start Navier-Stokes Muligrid Block
    
  //start Multigrid for UVWP
  nl_ml_prob.ClearMGIndex();
  nl_ml_prob.AddToMGIndex("U"); 
  nl_ml_prob.AddToMGIndex("V");
  nl_ml_prob.AddToMGIndex("P");
  
  // create Multigrid (PRLO, REST, MAT, VECs) based on MGIndex
  nl_ml_prob.CreateMGStruct();
  
  // create index of solutions to be to used in the Vanka Smoother  
  nl_ml_prob.ClearVankaIndex();
  nl_ml_prob.AddToVankaIndex("U"); 
  nl_ml_prob.AddToVankaIndex("V"); 
  nl_ml_prob.AddToVankaIndex("P"); 
    
  //Equation
  nl_ml_prob.AttachAssembleFunction(AssembleMatrixResNS);
  nl_ml_prob.SetNonLinearAlgorithm(true,"Newton",1.e-07);  //Navier-Stokes (Quasi-Newton - Newton)
  nl_ml_prob.SetMatrixProperties("Symmetric");
  nl_ml_prob.AddStabilization(true);
  
  //Solver Configuration 
  //Solver I (Gmres)
  nl_ml_prob.SetSmoother("Gmres");
  nl_ml_prob.SetTolerances(1.e-12,1.e-20,1.e+50,10);
  
 // Solver II (Vanka - MPSC)
//   nl_ml_prob.SetSmoother("Vanka");
//   nl_ml_prob.SetVankaSchurOptions(true,false,1);
//   nl_ml_prob.SetSolverFineGrids("GMRES");
//   nl_ml_prob.SetPreconditionerFineGrids("LU");
//   nl_ml_prob.SetTolerances(1.e-12,1.e-20,1.e+50,10);
//   nl_ml_prob.SetSchurTolerances(1.e-12,1.e-20,1.e+50,1);
//   nl_ml_prob.SetDimVankaBlock(6);                             //2^lev 1D 4^lev 2D 8^lev 3D
  
  // Solving
  nl_ml_prob.FullMultiGrid(10,1,1,"F-Cycle");
  
  // Delete Multigrid (PRLO, REST, MAT, VECs) based on MGIndex
  nl_ml_prob.DeleteMGStruct();
  
  /// End Navier-Stokes Muligrid Block
   
  //Post processing
//   double cforce[3];
//   nl_ml_prob.ComputeBdStress(4,cforce);
//   std::cout << "forcex:  " << cforce[0] << "   forcey: " << cforce[1] << std::endl;
//   
//   double cl[2];
//   cl[0] = (2.*cforce[0])/(1.*0.2*0.2*0.1);
//   cl[1] = (2.*cforce[1])/(1.*0.2*0.2*0.1);
//   std::cout << "drag_force:  " << cl[0] << "   lift_force: " << cl[1] << std::endl;
//   
  /// Print all solutions
  std::vector<std::string> print_vars;
  print_vars.resize(3);
  print_vars[0] = "U";
  print_vars[1] = "V";
  print_vars[2] = "P";
  
  bool DebugOn=1;
  //nl_ml_prob.printsol_gmv_binary("quadratic",nm-1,DebugOn);
  nl_ml_prob.printsol_gmv_binary("quadratic",nm,DebugOn);
  
  //nl_ml_prob.printsol_gmv_binary("linear",nm,DebugOn);
  //nl_ml_prob.printsol_gmv_binary("linear",nm-1,DebugOn);
  //nl_ml_prob.printsol_gmv_binary("linear",nm-2,DebugOn);
  
  nl_ml_prob.printsol_vtu_inline("biquadratic",print_vars);
  nl_ml_prob.printsol_vtu_inline("quadratic",print_vars);
  nl_ml_prob.printsol_vtu_inline("linear",print_vars);
  //nl_ml_prob.printsol_xdmf_hdf5("biquadratic",print_vars);
  
  /// Destroy the last PETSC objects
  nl_ml_prob.FreeMultigrid(); 
   
  delete [] infile;
  return(0);
}

//-----------------------------------------------------------------------------------------------------------------

bool SetRefinementFlag(const double &x, const double &y, const double &z, const int &ElemGroupNumber, const int &level) {
   bool refine=0;
   // refinemenet based on Elemen Group Number
   if(ElemGroupNumber==5 ) {
     refine=1;
   }
   if(ElemGroupNumber==6 && level<2) {
     refine=1;
   }
   if(ElemGroupNumber==7 ) {
     refine=0;
   }
// if(ElemGroupNumber==7 && level<=3) refine=1;
//    if(x>0 && x<5) refine=1;
   return refine;
}

//--------------------------------------------------------------------------------------------------------------

double InitVariables(const double &x, const double &y, const double &z,const char name[]){
  double value=0.;
  if(!strcmp(name,"U")){
   double um = 0.2;
   value=1.5*um*(4.0/(0.1681))*y*(0.41-y);
  }
  else if(!strcmp(name,"V")){
    value=0;
  } 
  else if(!strcmp(name,"W")){
    value=0;
  }
  else if(!strcmp(name,"P")){
    value=0;
  }
  return value;
}

//-------------------------------------------------------------------------------------------------------------------

bool SetBoundaryCondition(const double &x, const double &y, const double &z,const char name[], 
			  double &value, const int FaceName, const double time){
  bool test=1; //Dirichlet
  value=0.;
//   cout << "Time bdc : " <<  time << endl;
  if(!strcmp(name,"U")) {
    if(1==FaceName){   //inflow
     test=1;
     double um = 0.2; // U/Uref
     value=1.5*0.2*(4.0/(0.1681))*y*(0.41-y);
    }  
    else if(2==FaceName ){  //outflow
     test=0;
 //    test=1;
     value=0.;
    }
    else if(3==FaceName ){  // no-slip fluid wall
      test=1;
      value=0.;	
    }
    else if(4==FaceName ){  // no-slip solid wall
      test=1;
      value=0.;
    }
  }  
  else if(!strcmp(name,"V")){
    if(1==FaceName){            //inflow
      test=1;
      value=0.;
    }  
    else if(2==FaceName ){      //outflow
     test=0;
 //    test=1;
     value=0.;
    }
    else if(3==FaceName ){      // no-slip fluid wall
      test=1;
      value=0;
    }
    else if(4==FaceName ){      // no-slip solid wall
      test=1;
      value=0.;
    }
  }
  else if(!strcmp(name,"W")){
    if(1==FaceName){
      test=1;
      value=0.;
    }  
    else if(2==FaceName ){  
      test=1;
      value=0.;
    }
    else if(3==FaceName ){  
      test=1;
      value=0.;
    }
    else if(4==FaceName ){  
      test=1;
      value=0.;
    }
  }
  else if(!strcmp(name,"P")){
    if(1==FaceName){
      test=0;
      value=0.;
    }  
    else if(2==FaceName ){  
      test=0;
      value=0.;
    }
    else if(3==FaceName ){  
      test=0;
      value=0.;
    }
    else if(4==FaceName ){  
      test=0;
      value=0.;
    }
  }
  
  return test;
}

//------------------------------------------------------------------------------------------------------------

int AssembleMatrixResNS(NonLinearMultiLevelProblem &nl_ml_prob, unsigned level, const unsigned &gridn){
    
  clock_t AssemblyTime=0; 
  clock_t start_time, end_time;
  start_time=clock();
  
  //pointers and references
  LinearSolverM*     lsyspde_lev = nl_ml_prob.Lin_Solver_[level];
  LinearSolverM* lsyspdemesh_lev = nl_ml_prob.Lin_Solver_[level];
  elem*           myel     = lsyspdemesh_lev->_msh->el;
  Mat&           myKK      = lsyspde_lev->KK;
  Vec&           myRES     = lsyspde_lev->RES;
  vector <int>& myKKIndex  = lsyspde_lev->KKIndex; 
    
  // Allocation
  PetscInt node2[27];
  PetscInt node1[27];
  PetscInt nodeVAR[4][27];
  double B[4][4][27*27];
  double F[4][27];
  double coord[3][27];
  double phi2[27],gradphi2[27][3],Weight2;
  double normal[3];
  const double * phi1;
  PetscErrorCode ierr;  

  //data
  const unsigned dim = lsyspde_lev->_msh->GetDimension();
  unsigned nel=lsyspdemesh_lev->_msh->GetElementNumber();
  unsigned igrid=lsyspdemesh_lev->_msh->GetGridNumber();
  unsigned iproc = lsyspdemesh_lev->_msh->GetProcID();
  unsigned nprocs = lsyspdemesh_lev->_msh->GetNumProcs();
  double _ILambda= lsyspde_lev->GetCompressibility();
  double _IRe = nl_ml_prob._fluid->get_IReynolds_number();
  bool _penalty = lsyspde_lev->GetStabilization();
  const bool symm_mat = lsyspde_lev->GetMatrixProperties();
  const bool _NavierStokes = nl_ml_prob.GetNonLinearCase();
  unsigned _nwtn_alg = nl_ml_prob.GetNonLinearAlgorithm();
  bool _newton;
  if(_nwtn_alg==0) {
   _newton=0;
  } else {
    _newton=1;
  }
  
  //variable-name handling
  const char varname[4][2] = {"U","V","W","P"};
  const char coordname[3][2] = {"X","Y","Z"};
  unsigned indexVAR[4];
  unsigned indCOORD[3];
  unsigned indVAR[4];  
  double SolVAR[4];
  double gradSolVAR[3][3];
  for(unsigned ivar=0; ivar<dim; ivar++) {
    indexVAR[ivar]=nl_ml_prob.GetMGIndex(&varname[ivar][0]);
    indCOORD[ivar]=nl_ml_prob.GetIndex(&coordname[ivar][0]);
    indVAR[ivar]=nl_ml_prob.GetIndex(&varname[ivar][0]);
  }
  indexVAR[3]=nl_ml_prob.GetMGIndex(&varname[3][0]);
  indVAR[3]=nl_ml_prob.GetIndex(&varname[3][0]);
  
  //unknown order
  unsigned order_ind2 = lsyspde_lev->SolType[nl_ml_prob.GetIndex(&varname[0][0])];
  unsigned end_ind2   = lsyspde_lev->END_IND[order_ind2];
  unsigned order_ind1 = lsyspde_lev->SolType[nl_ml_prob.GetIndex(&varname[3][0])];
  unsigned end_ind1   = lsyspde_lev->END_IND[order_ind1];
  
  // Set to zeto all the entries of the matrix
  ierr = MatZeroEntries(myKK);  CHKERRQ(ierr);
  
  /// *** element loop ***
// for(int isdom=0; isdom<lsyspdemesh_lev->nsubdom; isdom++) {
  for(int isdom=iproc; isdom<iproc+1; isdom++) {
    for (int iel=lsyspdemesh_lev->_msh->IS_Mts2Gmt_elem_offset[isdom]; iel < lsyspdemesh_lev->_msh->IS_Mts2Gmt_elem_offset[isdom+1]; iel++) {

    unsigned kel = lsyspdemesh_lev->_msh->IS_Mts2Gmt_elem[iel];
    short unsigned kelt=myel->GetElementType(kel);
    unsigned nve2=myel->GetElementDofNumber(kel,end_ind2);
    unsigned nve1=myel->GetElementDofNumber(kel,end_ind1);
    
    //set to zero all the entries of the FE matrices
    for(int ivar=0; ivar<dim; ivar++) {
      memset(F[indexVAR[ivar]],0,nve2*sizeof(double));
      memset(B[indexVAR[ivar]][indexVAR[ivar]],0,nve2*nve2*sizeof(double));
      memset(B[indexVAR[ivar]][indexVAR[3]],0,nve2*nve1*sizeof(double));
      memset(B[indexVAR[3]][indexVAR[ivar]],0,nve1*nve2*sizeof(double));
    }
    memset(F[indexVAR[3]],0,nve1*sizeof(double));
    
    if(_nwtn_alg==2){
      for(int ivar=0; ivar<dim; ivar++) {
	for(int ivar2=1; ivar2<dim; ivar2++) {
	  memset(B[indexVAR[ivar]][indexVAR[(ivar+ivar2)%dim]],0,nve2*nve2*sizeof(double));
	}
      }
    }
  
    if(_penalty){
      memset(B[indexVAR[3]][indexVAR[3]],0,nve1*nve1*sizeof(double));
    }
    
    for( unsigned i=0;i<nve2;i++){
      unsigned inode=myel->GetElementVertexIndex(kel,i)-1u;	
      node2[i]=inode;
      unsigned inode_Metis=lsyspde_lev->_msh->GetMetisDof(inode,2);
      for(unsigned ivar=0; ivar<dim; ivar++) {
        coord[ivar][i]=(*lsyspde_lev->Sol_[indCOORD[ivar]])(inode_Metis);
	nodeVAR[ivar][i]=lsyspde_lev->GetKKDof(indVAR[ivar],indexVAR[ivar],inode);
      }
    }
    
    for(unsigned i=0;i<nve1;i++) {
      unsigned inode=(order_ind1<3)?(myel->GetElementVertexIndex(kel,i)-1u):(kel+i*nel);
      node1[i]=inode;
      nodeVAR[3][i]=lsyspde_lev->GetKKDof(indVAR[3],indexVAR[3],inode);
    }
   
    if(igrid==gridn || !myel->GetRefinedElementIndex(kel)) {
      // *** Gauss poit loop ***
      for(unsigned ig=0;ig < nl_ml_prob.type_elem[kelt][order_ind2]->GetGaussPointNumber(); ig++) {
	// *** get Jacobian and test function and test function derivatives ***
	(nl_ml_prob.type_elem[kelt][order_ind2]->*(nl_ml_prob.type_elem[kelt][order_ind2])->Jacobian_ptr)(coord,ig,Weight2,phi2,gradphi2);
	phi1=nl_ml_prob.type_elem[kelt][order_ind1]->GetPhi(ig);

	//velocity variable
	for(unsigned ivar=0; ivar<dim; ivar++) {
	  SolVAR[ivar]=0;
	  for(unsigned ivar2=0; ivar2<dim; ivar2++) gradSolVAR[ivar][ivar2]=0; 
	  unsigned SolIndex=nl_ml_prob.GetIndex(&varname[ivar][0]);
	  unsigned SolType=nl_ml_prob.GetSolType(&varname[ivar][0]);
	  for(unsigned i=0; i<nve2; i++) {
	    unsigned sol_dof = lsyspde_lev->_msh->GetMetisDof(node2[i],SolType);
	    double soli = (*lsyspde_lev->Sol_[SolIndex])(sol_dof);
	    SolVAR[ivar]+=phi2[i]*soli;
	    for(unsigned ivar2=0; ivar2<dim; ivar2++) gradSolVAR[ivar][ivar2] += gradphi2[i][ivar2]*soli; 
	  }
	}
	//pressure variable
	SolVAR[3]=0;
	unsigned SolIndex=nl_ml_prob.GetIndex(&varname[3][0]);
	unsigned SolType=nl_ml_prob.GetSolType(&varname[3][0]);
	for(unsigned i=0; i<nve1; i++){
	  unsigned sol_dof = lsyspde_lev->_msh->GetMetisDof(node1[i],SolType);
	  double soli = (*lsyspde_lev->Sol_[SolIndex])(sol_dof);
	  SolVAR[3]+=phi1[i]*soli;
	}

	// *** phi_i loop ***
	for(unsigned i=0; i<nve2; i++){
	
	  //BEGIN RESIDUALS A block ===========================
	  
	  for(unsigned ivar=0; ivar<dim; ivar++) {
	    double Adv_rhs=0;
	    double Lap_rhs=0;
	    for(unsigned ivar2=0; ivar2<dim; ivar2++) {
	      Lap_rhs += gradphi2[i][ivar2]*gradSolVAR[ivar][ivar2];
	      Adv_rhs += SolVAR[ivar2]*gradSolVAR[ivar][ivar2];
	    }
	    F[indexVAR[ivar]][i]+= (-_IRe*Lap_rhs-_NavierStokes*Adv_rhs*phi2[i]+SolVAR[3]*gradphi2[i][ivar])*Weight2; 
	  }
	  
	  //END RESIDUALS A block ===========================

	  // *** phi_j loop ***
	  for(unsigned j=0; j<nve2; j++) {
		
	    double Lap=0;
	    double Adv1=0;
	    double Adv2 = phi2[i]*phi2[j]*Weight2;
	    for(unsigned ivar=0; ivar<dim; ivar++) {
	      // Laplacian
	      Lap  += gradphi2[i][ivar]*gradphi2[j][ivar]*Weight2;
	      // advection term I
	      Adv1 += SolVAR[ivar]*gradphi2[j][ivar]*phi2[i]*Weight2;
	    }

	    for(unsigned ivar=0; ivar<dim; ivar++) {    
	      B[indexVAR[ivar]][indexVAR[ivar]][i*nve2+j] += _IRe*Lap + _NavierStokes*_newton*Adv1;
	      if(_nwtn_alg==2){
		// Advection term II
		B[indexVAR[ivar]][indexVAR[ivar]][i*nve2+j]       += Adv2*gradSolVAR[ivar][ivar];
		for(unsigned ivar2=1; ivar2<dim; ivar2++) {
		  B[indexVAR[ivar]][indexVAR[(ivar+ivar2)%dim]][i*nve2+j] += Adv2*gradSolVAR[ivar][(ivar+ivar2)%dim];
		}
	      }
	    }
  	    
	  }   //end phij loop
	} //end phii loop
  

	// *** phi_i loop ***
	for(unsigned i=0; i<nve2; i++){
	  // *** phi_j loop ***
	  for(unsigned j=0; j<nve1; j++){
	    for(unsigned ivar=0; ivar<dim; ivar++) {
	      B[indexVAR[ivar]][indexVAR[3]][i*nve1+j] -= gradphi2[i][ivar]*phi1[j]*Weight2;
	    }
	  }
	} 
	
	// *** phi_i loop ***
	for(unsigned i=0; i<nve1; i++){
	  //BEGIN RESIDUALS B block ===========================
	  
	  double div = 0;
	  for(unsigned ivar=0; ivar<dim; ivar++) div += gradSolVAR[ivar][ivar];
	  F[indexVAR[3]][i]+= (phi1[i]*div +_penalty*_ILambda*phi1[i]*SolVAR[3])*Weight2;
	  	  	    
	  //END RESIDUALS  B block ===========================

	  // *** phi_j loop ***
	  for(unsigned j=0; j<nve2; j++) {
	    for(unsigned ivar=0; ivar<dim; ivar++) {
	      B[indexVAR[3]][indexVAR[ivar]][i*nve2+j]-= phi1[i]*gradphi2[j][ivar]*Weight2;
	    }
	  }  //end phij loop
	}  //end nve1
	
	if(_penalty){  //block nve1 nve1
	  // *** phi_i loop ***
	  for(unsigned i=0; i<nve1; i++){
	    // *** phi_j loop ***
	    for(unsigned j=0; j<nve1; j++){
	      B[indexVAR[3]][indexVAR[3]][i*nve1+j]-=_ILambda*phi1[i]*phi1[j]*Weight2;
	    }
	  }
	}   //end if penalty
      }  // end gauss point loop
      
	
//--------------------------------------------------------------------------------------------------------
// Boundary Integral
    //number of faces for each type of element
    if (igrid==gridn || !myel->GetRefinedElementIndex(kel) ) {
     
	unsigned nfaces = myel->GetElementFaceNumber(kel);

	// loop on faces
	for(unsigned jface=0;jface<nfaces;jface++){ 
	  
	  // look for boundary faces
	  if(myel->GetFaceElementIndex(kel,jface)<0){
	    for(unsigned ivar=0; ivar<dim; ivar++) {
	      nl_ml_prob.ComputeBdIntegral(&varname[ivar][0], kel, jface, level, ivar);
	    }
          }
        }
    }
//--------------------------------------------------------------------------------------------------------
    
    }  // endif single element not refined or fine grid loop
   
   
   //--------------------------------------------------------------------------------------------------------
   //Sum the small matrices into the Big Matrix
    for(unsigned ivar=0; ivar<dim; ivar++) {
      ierr = VecSetValues(myRES,nve2,nodeVAR[ivar],F[indexVAR[ivar]],ADD_VALUES);CHKERRQ(ierr);
      ierr = MatSetValuesBlocked(myKK,nve2,nodeVAR[ivar],nve2,nodeVAR[ivar],
				 B[indexVAR[ivar]][indexVAR[ivar]],ADD_VALUES);CHKERRQ(ierr);
      ierr = MatSetValuesBlocked(myKK,nve2,nodeVAR[ivar],nve1,nodeVAR[3],B[indexVAR[ivar]][indexVAR[3]],
				 ADD_VALUES);CHKERRQ(ierr);
      ierr = MatSetValuesBlocked(myKK,nve1,nodeVAR[3],nve2,nodeVAR[ivar],B[indexVAR[3]][indexVAR[ivar]],
				 ADD_VALUES);CHKERRQ(ierr);
      if(_nwtn_alg==2){
	for(unsigned ivar2=1; ivar2<dim; ivar2++) {
	  ierr = MatSetValuesBlocked(myKK,nve2,nodeVAR[ivar],nve2,nodeVAR[(ivar+ivar2)%dim],B[indexVAR[ivar]][indexVAR[(ivar+ivar2)%dim]],
				   ADD_VALUES);CHKERRQ(ierr);
	}
      }
    }
     
    //Penalty
    if(_penalty){
      ierr = MatSetValuesBlocked(myKK,nve1,nodeVAR[3],nve1,nodeVAR[3],B[indexVAR[3]][indexVAR[3]],ADD_VALUES);CHKERRQ(ierr);
    }
    ierr = VecSetValues(myRES,nve1,nodeVAR[3],F[indexVAR[3]],ADD_VALUES);CHKERRQ(ierr);
    //--------------------------------------------------------------------------------------------------------  
    
    } //end list of elements loop for each subdomain
  } //end list of subdomain
  

  //BEGIN MATRIX ASSEMBLY ============ 
  ierr = MatAssemblyBegin(myKK,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(myKK,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  //END MATRIX ASSEMBLY   ============ 
  
  
//    PetscViewer viewer;
// // PetscViewerSetFormat(viewer,PETSC_VIEWER_DEFAULT);
//    PetscViewerDrawOpen(PETSC_COMM_WORLD,PETSC_NULL,PETSC_NULL,0,0,700,700,&viewer);
//    MatView(myKK,viewer);
//    double ff;
//    std::cin>>ff;

  
   //BEGIN RESIDUAL ASSEMBLY ============ 
  ierr = VecAssemblyBegin(myRES);CHKERRQ(ierr);
  ierr = VecAssemblyEnd(myRES);CHKERRQ(ierr);
  
  if(nprocs!=1) {
//     ierr = VecGhostUpdateBegin(myRES,ADD_VALUES,SCATTER_REVERSE);CHKERRQ(ierr);
//     ierr = VecGhostUpdateEnd(myRES,ADD_VALUES,SCATTER_REVERSE);CHKERRQ(ierr);
    ierr = VecGhostUpdateBegin(myRES,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
    ierr = VecGhostUpdateEnd(myRES,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
  }
  //END RESIDUAL ASSEMBLY   ============ 
 
  // *************************************
  end_time=clock();
  AssemblyTime+=(end_time-start_time);
  // ***************** END ASSEMBLY *******************
  
  return ierr;
  
}


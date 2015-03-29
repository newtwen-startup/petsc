
static char help[] = "Tests PetscViewerHDF5 VecView()/VecLoad() function.\n\n";

#include <petscviewer.h>
#include <petscviewerhdf5.h>
#include <petscvec.h>

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **args)
{
  PetscErrorCode ierr;
  Vec            x,y;
  PetscReal      norm;
  const char     *vecname;
  PetscViewer    H5viewer;

  PetscInitialize(&argc,&args,(char*)0,help);
  ierr = VecCreate(PETSC_COMM_WORLD,&x);CHKERRQ(ierr);
  ierr = VecSetFromOptions(x);CHKERRQ(ierr);
  ierr = VecSetSizes(x,11,PETSC_DETERMINE);CHKERRQ(ierr);
  ierr = VecSet(x,22.3);CHKERRQ(ierr);

  ierr = PetscViewerHDF5Open(PETSC_COMM_WORLD,"x.h5",FILE_MODE_WRITE,&H5viewer);CHKERRQ(ierr);
  ierr = PetscViewerSetFromOptions(H5viewer);CHKERRQ(ierr);
  ierr = VecView(x,H5viewer);CHKERRQ(ierr);
  ierr = PetscViewerDestroy(&H5viewer);CHKERRQ(ierr);

  ierr = VecDuplicate(x,&y);CHKERRQ(ierr);
  ierr = PetscObjectGetName((PetscObject)x,&vecname);CHKERRQ(ierr);
  ierr = PetscObjectSetName((PetscObject)y,vecname);CHKERRQ(ierr);

  /* Create the HDF5 viewer for reading */
  ierr = PetscViewerHDF5Open(PETSC_COMM_WORLD,"x.h5",FILE_MODE_READ,&H5viewer);CHKERRQ(ierr);
  ierr = PetscViewerSetFromOptions(H5viewer);CHKERRQ(ierr);
  ierr = VecLoad(y,H5viewer);CHKERRQ(ierr);
  ierr = PetscViewerDestroy(&H5viewer);CHKERRQ(ierr);

  ierr = VecAXPY(y,-1.0,x);CHKERRQ(ierr);
  ierr = VecNorm(y,NORM_2,&norm);CHKERRQ(ierr);
  if (norm > 1.e-6) SETERRQ(PETSC_COMM_WORLD,PETSC_ERR_PLIB,"Vec read in does not match vector written out");

  ierr = VecDestroy(&y);CHKERRQ(ierr);
  ierr = VecDestroy(&x);CHKERRQ(ierr);
  ierr = PetscFinalize();
  return 0;
}

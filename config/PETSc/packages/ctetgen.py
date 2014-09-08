import config.package
import os

class Configure(config.package.GNUPackage):
  def __init__(self, framework):
    config.package.GNUPackage.__init__(self, framework)
    self.gitcommit         = 'f225517cc660ca356e6d8ee8585ea31345702dc6'
    self.giturls           = ['https://bitbucket.org/petsc/ctetgen.git']
    self.download          = ['http://ftp.mcs.anl.gov/pub/petsc/externalpackages/ctetgen-0.1.tar.gz']
    self.functions         = []
    self.includes          = []
    return

  def Install(self):
    try:
      self.installDirProvider.printSudoPasswordMessage(1)
      output,err,ret  = config.package.GNUPackage.executeShellCommand(self.installDirProvider.installSudo+'mkdir -p '+os.path.join(self.installDir,'include'),timeout=1000, log = self.framework.log)
      output,err,ret  = config.package.GNUPackage.executeShellCommand(self.installDirProvider.installSudo+'mkdir -p '+os.path.join(self.installDir,'lib'),timeout=1000, log = self.framework.log)
      output,err,ret  = config.package.GNUPackage.executeShellCommand(self.installDirProvider.installSudo+'cp '+os.path.join(self.packageDir,'ctetgen.h')+' '+os.path.join(self.installDir,'include'),timeout=1000, log = self.framework.log)
      self.framework.log.write(output+err)
    except RuntimeError, e:
      raise RuntimeError('Error running make on Ctetgen: '+str(e))
    return self.installDir

  def configureLibrary(self):
    ''' Just assume the downloaded library will work'''
    self.getInstallDir()
    self.include = [os.path.join(self.installDir,'include')]
    self.lib     = [os.path.join(self.installDir,'lib','libctetgen.a')]
    self.found   = 1
    self.dlib    = self.lib
    if not hasattr(self.framework, 'packages'):
      self.framework.packages = []
    self.framework.packages.append(self)

  def postProcess(self):
    try:
      self.logPrintBox('Compiling Ctetgen; this may take several minutes')
      # uses the regular PETSc library builder and then moves result 
      output,err,ret  = config.package.GNUPackage.executeShellCommand('rm -rf '+os.path.join(self.installDirProvider.confDir,'lib','libpetsc.a')+' && cd '+self.packageDir+' && '+self.make.make+' && '+self.installDirProvider.installSudo+'mv '+os.path.join(self.installDirProvider.confDir,'lib','libpetsc.a')+' '+os.path.join(self.installDir,'lib','libctetgen.a'),timeout=1000, log = self.framework.log)
      self.framework.log.write(output+err)
    except RuntimeError, e:
      raise RuntimeError('Error running make on Ctetgen: '+str(e))



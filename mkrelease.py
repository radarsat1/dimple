#!/usr/bin/env python

# Script to handle automation of releases for DIMPLE.
# This script is tailored to DIMPLE's particular subversion repository and web site.

import sys, os, re, pprint, subprocess

dryrun = True

settings = {
    'PROJECT_NAME':'dimple',
    'SVN2CL_ARGS':'--group-by-day -r HEAD:400',
    'TMP':os.getenv('HOME')+'/Desktop/',

    # Update these variables when URLs change
    'SVN_URL':'svn+ssh://sinclairs@132.206.14.8/Volumes/home/sinclairs/.svn_repo/',
    'SVN_TRUNK':'trunk/school/thesis/projects/osc/implementations/chai3d',
    'SVN_TAGS':'tags/dimple',
    'RELEASE_FOLDER':'sinclair@sound.music.mcgill.ca:public_html/pub/dimple/',
    'RELEASE_DOCUWIKI':'sinclair@sound.music.mcgill.ca:public_html/content/data/pages/dimple.txt',
        
    # Update these variables when included files change
    # Ensure these files are present in the project folder
    'ALL_EXTRA':'ChangeLog README.html',
    'ALL_EXCLUDE':'mkrelease.py',
    'SRC_EXTRA':'',
    'SRC_EXCLUDE':'',
    'WIN32_EXTRA':'dimple.exe icon/dimple_sphere.png',
    'WIN32_TEXT':'README README.html LICENSE TODO ChangeLog'  # these will be translated with unix2dos & renamed to .txt
    }

class Project:
    def __init__(self, settings, dryrun):
        self.settings = settings
        self.dryrun = dryrun
        self.uploaded = []
        for s in ['ALL_EXTRA', 'ALL_EXCLUDE',
                  'SRC_EXTRA', 'SRC_EXCLUDE',
                  'WIN32_EXTRA','WIN32_TEXT']:
            if self.settings.has_key(s):
                self.settings[s] = self.settings[s].split()
        print '== Project:', self.settings['PROJECT_NAME']

    def run(self, cmd, drycmd, leave_stdout=False):
        if isinstance(cmd, str): cmd = cmd.split()
        if not drycmd:
            print ' '.join(cmd)
            if self.dryrun:
                return [[''], 0]
        if (leave_stdout):
            p = subprocess.Popen(cmd, stderr=subprocess.PIPE)
        else:
            p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        ff = p.communicate()
        rc = []
        if ff[0]:
            rc += ff[0].splitlines()
        if ff[1]:
            rc += ff[1].splitlines()
        return [rc, p.returncode]

    def run_check(self, cmd, drycmd, intro, error, leave_stdout=False):
        print intro,'...',
        if not drycmd:
            print '(dry)\n  ',
        sys.stdout.flush()
        rc = self.run(cmd, drycmd, leave_stdout)
        if not drycmd:
            print '  ',
        if rc[1]!=0:
            print 'error'
            print '\n'.join(rc[0])
            print error
            sys.exit(1)
        print 'done.'
        return rc[0]

    def gather_information(self):
        self.release_number = self.get_last_release()
        print 'Last release was', self.relnum()
        self.release_number[2]+=1
        rel = raw_input('Enter the number for this release [default %s]: '%self.relnum())
        if rel!='':
            pat = re.compile('((\d+).(\d+).(\d+))')
            m = re.match(pat, rel)
            if not m:
                print 'Not a valid release number. Must be of the form "X.Y.Z".'
                sys.exit(1)
            self.release_number = [int(m.group(2)),int(m.group(3)),int(m.group(4))]
        print 'Working on release', self.relnum()

    def tags(self):
        return self.settings['SVN_URL']+self.settings['SVN_TAGS']

    def trunk(self):
        return self.settings['SVN_URL']+self.settings['SVN_TRUNK']

    def relnum(self):
        return '.'.join([str(i) for i in self.release_number])

    def relname(self):
        return self.settings['PROJECT_NAME']+'-'+self.relnum()

    def relname_win32(self):
        return self.relname()+'-win32'

    def get_last_release(self):
        # Read contents of tag folder
        pat = re.compile(self.settings['PROJECT_NAME']+'-((\d+).(\d+).(\d+))/?.*')
        ls = self.run_check('svn ls '+self.tags(), True,
                            "Reading tag folder",
                            "Error reading tag folder")
        ls = [re.match(pat, l) for l in ls]

        # Get the highest version number
        vers = []
        for m in ls:
            if (m): vers.append([int(m.group(2)),int(m.group(3)),int(m.group(4))])
        for i in range(3):
            num = max([v[i] for v in vers])
            tmp=[]
            for v in vers:
                if v[i]>=num:
                    tmp.append(v)
            vers = tmp
            if len(vers)==1:
                break;
        if len(vers)!=1:
            print "Couldn't determine version number."
            sys.exit(1)
        return vers[0]

    def check_files(self):
        files = []
        for f in self.settings['WIN32_EXTRA']:
            if not os.access(f,os.F_OK):
                files.append(f)
        if not len(files)==0:
            print 'The following files are missing:'
            for f in files: print ' ',f
            sys.exit(1)
        files = []
        for f in self.settings['WIN32_EXTRA']:
            if not os.access(f,os.R_OK):
                files.append(f)
        if not len(files)==0:
            print 'Permission problems on these files:'
            for f in files: print ' ',f
            sys.exit(1)

        # Check that we can write to temp directory
        test = self.settings['TMP']+'mkrelease-test.tmp'
        self.run_check(['bash','-c','echo test >'+test], True,
                       "Testing temp directory", "Temp directory not writable!")
        self.run(['rm','-f',test], True)
        return True

    def check_svn_status(self):
        rc = self.run_check(['svn','status'], True,
                            "Checking project status",
                            "Error getting project status.")
        rc = [r.split() for r in rc]
        for r in rc:
            if r[0]!='?':
                print 'The project status is not up to date.  Please perform a commit.'
                sys.exit(1)

    def do_tag_release(self):
        raw_input('Tagging current folder. Press enter to continue or Ctrl-C to quit: ')
        rc = self.run_check(['svn','cp', self.trunk(),
                             self.tags()+'/'+self.relname()], False,
                            "Tagging", "Error tagging release.", leave_stdout=True)

    def do_changelog(self):
        self.run_check('svn2cl '+self.settings['SVN2CL_ARGS'], True,
                       "Generating ChangeLog",
                       "Error generating ChangeLog.")
        
    def do_source_release(self):
        # Determine what's actually in the svn repo
        ls = self.run_check(['svn','ls','-R'], True,
                            'Reading repository contents',
                            'Error reading repository contents.')
        dirs=[]
        files=[]
        for l in ls:
            if len(l)>0 and l[-1]=='/':
                dirs.append(l)
            elif len(l)>0:
                files.append(l)

        if self.settings.has_key('ALL_EXTRA'):
            for f in settings['ALL_EXTRA']:
                if not f in files: files.append(f)
        if self.settings.has_key('ALL_EXCLUDE'):
            for f in self.settings['ALL_EXCLUDE']:
                if f in files: files.remove(f)

        if self.settings.has_key('SRC_EXTRA'):
            for f in settings['SRC_EXTRA']:
                if not f in files: files.append(f)
        if self.settings.has_key('SRC_EXCLUDE'):
            for f in self.settings['SRC_EXCLUDE']:
                if f in files: files.remove(f)

        # Setup temporary directory
        tmpdir = self.settings['TMP']+self.relname()
        self.run_check(['mkdir',tmpdir], True,
                       'Creating source folder',
                       "Couldn't create source folder.")

        # Setup directories
        for d in dirs:
            self.run_check(['mkdir',tmpdir+'/'+d], True,
                           'Creating '+d,"Couldn't create folder "+d)

        # Copy files
        for f in files:
            self.run_check(['cp',f,tmpdir+'/'+f], True,
                           'Copying '+f,"Couldn't copy "+f)

        # Create archive
        arc = self.relname()+'.tar.bz2'
        self.run_check(['tar','-C',self.settings['TMP'],'-cjf',arc,self.relname()], True,
                       'Creating archive '+arc,"Couldn't create "+arc)

        # Delete temporary folder
        self.run_check(['rm','-rf',tmpdir], True,
                       'Deleting temporary folder',"Couldn't delete "+tmpdir)

        # Upload to a web site
        self.run_check(['scp',arc,self.settings['RELEASE_FOLDER']], False,
                       'Uploading '+arc,"Error uploading "+arc)
        self.uploaded.append(self.settings['RELEASE_FOLDER']+arc)

    def do_ubuntu_release(self):
        # TODO
        pass

    def do_win32_release(self):
        files=self.settings['WIN32_EXTRA'] + self.settings['WIN32_TEXT']
        
        if self.settings.has_key('ALL_EXTRA'):
            for f in settings['ALL_EXTRA']:
                if not f in files: files.append(f)
        if self.settings.has_key('ALL_EXCLUDE'):
            for f in self.settings['ALL_EXCLUDE']:
                if f in files: files.remove(f)

        dirs=[]
        for f in files:
            i = f.rfind('/')
            if (i>=0):
                d = f[:i]
                if not d in dirs:
                    dirs.append(d)                

        # Setup temporary directory
        tmpdir = self.settings['TMP']+self.relname()+'-win32'
        self.run_check(['mkdir',tmpdir], True,
                       'Creating source folder',
                       "Couldn't create source folder.")

        # Setup directories
        for d in dirs:
            self.run_check(['mkdir',tmpdir+'/'+d], True,
                           'Creating '+d,"Couldn't create folder "+d)

        # Copy files
        for f in files:
            self.run_check(['cp',f,tmpdir+'/'+f], True,
                           'Copying '+f,"Couldn't copy "+f)

        # Create archive
        basename = self.relname()+'-win32'
        arc = basename+'.zip'
        self.run_check(['bash','-c','cd '+self.settings['TMP']+'; zip -r -9 '+
                        os.getenv('PWD')+'/'+arc+' '+basename], True,
                       'Creating archive '+arc,"Couldn't create "+arc)

        # Delete temporary folder
        self.run_check(['rm','-rf',tmpdir], True,
                       'Deleting temporary folder',"Couldn't delete "+tmpdir)

        # Upload to a web site
        self.run_check(['scp',arc,self.settings['RELEASE_FOLDER']], False,
                       'Uploading '+arc,"Error uploading "+arc)
        self.uploaded.append(self.settings['RELEASE_FOLDER']+arc)

    def do_update_wiki(self):
#        print 'Uploaded:'
#        print '\n'.join(self.uploaded)

        server = self.settings['RELEASE_DOCUWIKI'].split(':')[0]
        page = self.settings['RELEASE_DOCUWIKI'].split(':')[1]

        # Use sed on the server to modify the wiki page with the new version number
        self.run_check(['ssh',server,'sed',"'s/"+self.settings['PROJECT_NAME']
                        + "-\([0-9]*\.[0-9]*\.[0-9]*\)/'"
                        + self.relname()+'/g',page,'--in-place'], False,
                       "Updating docuwiki page", "Error updating docuwiki page.")

if __name__=="__main__":
    if '--dry-run' in sys.argv:
        dryrun = True
    if '--no-dry-run' in sys.argv:
        dryrun = False
    project = Project(settings, dryrun)
    if not '--no-status' in sys.argv:
        project.check_svn_status()
    project.check_files()
    project.gather_information()
    if not '--no-tag' in sys.argv:
        project.do_tag_release()
    if not '--no-changelog' in sys.argv:
        project.do_changelog()
    if not '--no-src' in sys.argv:
        project.do_source_release()
    if not '--no-ubuntu' in sys.argv:
        project.do_ubuntu_release()
    if not '--no-win32' in sys.argv:
        project.do_win32_release()
    if not '--no-wiki' in sys.argv:
        project.do_update_wiki()


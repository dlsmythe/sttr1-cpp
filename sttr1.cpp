#!/usr/bin/env python
'''
'''

from random import *
from time import *
from math import *

KLINGON_MAX_SHIELDS=200

"MAX_STARDATES: maximum number of stardates the mission contains"
MAX_STARDATES=30

#"C[]: course array"
#" X|  0 -1 -1 -1  0  1  1  1  0"
#" Y|  1  1  0 -1 -1 -1  0  1  1"
C = {}
#C[1,1]=C[2,1]=C[3,1]=C[3,2]=C[4,2]=C[5,2]=-1
#C[0,1]=C[2,2]=C[4,1]=C[6,2]=C[8,1]=0
#C[0,2]=C[1,2]=C[5,1]=C[6,1]=C[7,1]=C[7,2]=C[8,2]=1
C[2,1]=C[3,1]=C[4,1]=C[4,2]=C[5,2]=C[6,2]=-1
C[1,1]=C[3,2]=C[5,1]=C[7,2]=C[9,1]=0
C[1,2]=C[2,2]=C[6,1]=C[7,1]=C[8,1]=C[8,2]=C[9,2]=1

class SectorPos(object):

    def __init__(self, row, col):
        self.row = int(row+.5)
        self.col = int(col+.5)

    def row(self):
        return self.row

    def col(self):
        return self.col

    def distance_to(self, pos):
        return sqrt(pow(pos.row-self.row,2)+pow(pos.col-self.col,2))
    
class Sector(object):

    SPACE=0
    STAR=1
    STARBASE=2
    STARSHIP=3
    KLINGON=4

    def __init__(self, row, col, type=SPACE):
        self.pos = SectorPos(row, col)
        self.type = type

    def glyph(self):
        return '   '

class Klingon(Sector):

    def __init__(self, row, col):
        Sector.__init__(self, row, col, self.KLINGON)
        self.shields = KLINGON_MAX_SHIELDS

    def glyph(self):
        return '+++'

class Starship(Sector):

    def __init__(self, row, col):
        Sector.__init__(self, row, col, self.STARSHIP)

    def glyph(self):
        return '<*>'
    
class Starbase(Sector):

    def __init__(self, row, col):
        Sector.__init__(self, row, col, self.STARBASE)

    def glyph(self):
        return '>!<'

class Star(Sector):

    def __init__(self, row, col):
        Sector.__init__(self, row, col, self.STAR)

    def glyph(self):
        return ' * '
        
class Quadrant(object):
    # PYTHON: a quadrant is a dictionary indexed by a row,col pair.  Each element is a 'sector'

    def __init__(self, r, c):
        #print 'INITIALIZING QUADRANT %d,%d' % (r,c)
        self.r = r
        self.c = c
        self.sectors = {}
        for row in range(8):
            for col in range(8):
                self.sectors[row,col] = Sector(row,col)
        self.num_klingons = 0
        self.num_starbases = 0
        self.num_stars = 0
 
       #"select # klingons for this quadrant, and add to total"
        #print 'POPULATING QUADRANT %d,%d' % (self.r,self.c)
        R1=randint(1,100)
        if R1>98:
            self.num_klingons = 3
        elif R1>95:
            self.num_klingons = 2
        elif R1>80:
            self.num_klingons = 1
        else:
            self.num_klingons = 0

        for i in range(self.num_klingons):
            r,c = self.randomemptypos()
            self.sectors[r,c] = Klingon(r,c)
                    
        #"select # starbases for this quadrant, and add to total"
        self.num_starbases = 0 if randint(1,100) <= 96 else 1
        for i in range(self.num_starbases):
            r,c = self.randomemptypos()
            self.sectors[r,c] = Starbase(r,c)

        #"select # of stars for this quadrant"
        self.num_stars = randint(1,8)
        for i in range(self.num_stars):
            r,c = self.randomemptypos()
            self.sectors[r,c] = Star(r,c)

    def emptypos(self):
        for r in range(8):
            for c in range(8):
                if (self.sectors[r,c].type == Sector.SPACE):
                    return r,c;

    def randomemptypos(self):
        while True:
            r=randint(0,7)
            c=randint(0,7)
            if (self.sectors[r,c].type == Sector.SPACE):
                return r,c;

    def nearestempty(self, shippos):
        mind = 20
        minp = None
        for r in range(8):
            for c in range(8):
                s = self.sectors[r,c] 
                if s.type != Sector.SPACE:
                    continue
                d = shippos.distance_to(s.pos)
                if d < mind:
                    mind = d
                    minp = s.pos
        return minp
    
    def sector(self,row,col):
        return self.sectors[int(row),int(col)] if (row >= 0 and row < 8 and col >= 0 and col < 8) else None

    def setsector(self, sec, adjust=False):
        cur = self.sectors[sec.pos.row, sec.pos.col]
        if sec.type == Sector.STARSHIP:
            if cur.type != Sector.SPACE:
                if not adjust:
                    print 'Landed on occupied space. Enterprise destroyed.'
                    sys.exit(0)
                sec.pos = self.nearestempty(sec.pos)
            print 'Placing STARSHIP at sector position {0},{1}'.format(sec.pos.row, sec.pos.col)
            for r in range(8):
                for c in range(8):
                    s = self.sectors[r,c]
                    if s.type == Sector.KLINGON:
                        s.shields = KLINGON_MAX_SHIELDS
                        print 'klingon at %d,%d in quadrant %d,%d' % (r,c, self.r, self.c)
                    elif s.type == Sector.STARSHIP:
                        print 'starship already in sector at position {0},{1}'.format(r,c)
        elif sec.type == Sector.KLINGON and cur.type == Sector.SPACE:
            self.num_klingons += 1
        elif cur.type == Sector.KLINGON and sec.type == Sector.SPACE:
            self.num_klingons -= 1
        elif cur.type == Sector.STARBASE and sec.type == Sector.SPACE:
            self.num_starbases -= 1
        self.sectors[sec.pos.row, sec.pos.col] = sec

    def klingons(self):
        ks = []
        for r in range(8):
            for c in range(8):
                if self.sectors[r,c].type == Sector.KLINGON:
                    ks.append(self.sectors[r,c])
        return ks

class Galaxy(object):
    # the galaxy is a dictionary indexed by a row,col pair. Each element is a 'quadrant'.

    galaxy = None
    
    def __init__(self):
        #print 'INITIALIZING GALAXY'
        self.quadrants = {}
        self.total_klingons = 0

        # initialize the galaxy
        #"have to have at least 1 each of klingons and starbases"
        self.remaining_klingons = 0
        self.remaining_starbases = 0
        while self.remaining_starbases <= 0 or self.remaining_klingons <= 0:
            self.remaining_klingons = 0
            self.remaining_starbases = 0
            for I in range(8):
                for J in range(8):
                    self.quadrants[I,J] = Quadrant(I, J)
                    self.remaining_klingons +=  self.quadrants[I,J].num_klingons
                    self.remaining_starbases += self.quadrants[I,J].num_starbases

        #"save the original total number of klingons for the efficiency rating at the end of the game"
        self.total_klingons = self.remaining_klingons

    def quadrant(self,row,col):
        return self.quadrants[row,col]
    
class Ship(object):

    systems = [ 'warpdrive', 'srsensors', 'lrsensors', 'phasers', 'photons', 'damagectl', 'shields', 'computer', 'computerctl' ]

    system_names = {
        'warpdrive' : "WARP ENGINES",
        'srsensors' : "S.R. SENSORS",
        'lrsensors' : "L.R. SENSORS",
        'phasers'   : "PHASER CNTRL",
        'photons'   : "PHOTON TUBES",
        'damagectl' : "DAMAGE CNTRL",
        'shields'   : "SHIELD CNTRL",
        'computer'  : "COMPUTER",
        'computerctl' : "COMPT. CNTRL"
        }

    #"MAXPHOTONS: full magazine of torpedos"
    MAXPHOTONS = 10

    #"MAXENERGY: full charge of energy"
    MAXENERGY = 3000

    def __init__(self):
        self.DOCKED = False
        self.curdamage = {}
        for s in self.systems:
            self.curdamage[s] = 0
        self.photons = self.MAXPHOTONS
        self.energy = self.MAXENERGY
        self.shields = 0
        self.clear_damage()
        self.condition = 'GREEN'
        self.computer_galaxy_scan = Galaxy()

        #"Q1 and Q2: X and Y coordinates of the current quadrant, 0..7 each"
        self.Q1=randint(0,7)
        self.Q2=randint(0,7)

        #"S1 and S2, X and Y coordinates of the ship in the current quadrant, 1..8 each"
        r, c = Galaxy.galaxy.quadrant(self.Q1,self.Q2).randomemptypos()
        self.pos = SectorPos(r,c)

    def srscan(self, gamestate):
        "short-range sensor scan"

        quadrant = Galaxy.galaxy.quadrant(self.Q1,self.Q2)
        sector = quadrant.sector(self.pos.row, self.pos.col)
        if not sector:
            print "SR SENSORS INOPERATIVE"
            print "srow {0} scol {1}".format(self.pos.row, self.pos.col)
            return

        # XXX debug
        for r in range(8):
            for c in range(8):
                if quadrant.sector(r,c).type == Sector.STARSHIP:
                    print "starship at {0},{1}".format(r,c)

        # see if we're docked
        self.DOCKED=False
        I=self.pos.row-1
        while I <= self.pos.row+1 and not self.DOCKED:
            J=self.pos.col-1
            while J <= self.pos.col+1 and not self.DOCKED:
                if I>=0 and I<=7 and J>=0 and J<=7:
                    s = quadrant.sector(self.pos.row, self.pos.col)
                    if s and s.type == Sector.STARBASE:
                        self.energy = self.MAXENERGY
                        self.photons = self.MAXPHOTONS
                        self.clear_damage()
                        print "SHIELDS DROPPED FOR DOCKING PURPOSES"
                        self.shields = 0
                        self.DOCKED=True
                J=J+1
            I=I+1

        #"selectCondition"
        if self.DOCKED:
            self.condition="DOCKED"
        elif quadrant.num_klingons>0:
            self.condition="RED"
        elif self.energy < self.MAXENERGY*.1:
            self.condition="YELLOW"
        else:
            self.condition="GREEN"

        #"printQuadrant"
        if self.damage('srsensors') < 0:
            print 
            print "*** SHORT RANGE SENSORS ARE OUT ***"
            print 
        else:
            print '---------------------------------------'
            for col in range(8):
                print (' %3s' % quadrant.sector(0,col).glyph()),
            print
            for col in range(8):
                print (' %3s' % quadrant.sector(1,col).glyph()),
            print '        STARDATE        	%d' % (gamestate.T)
            for col in range(8):
                print (' %3s' % quadrant.sector(2,col).glyph()),
            print '        CONDITION       	%s' % (self.condition)
            for col in range(8):
                print (' %3s' % quadrant.sector(3,col).glyph()),
            print '        QUADRANT        	%d,%d' % (self.Q1, self.Q2)
            for col in range(8):
                print (' %3s' % quadrant.sector(4,col).glyph()),
            print '        SECTOR          	%d,%d' % (self.pos.row, self.pos.col)
            for col in range(8):
                print (' %3s' % quadrant.sector(5,col).glyph()),
            print '        ENERGY          	%d' % (self.energy)
            for col in range(8):
                print (' %3s' % quadrant.sector(6,col).glyph()),
            print '        PHOTON TORPEDOES        %d' % (self.photons)
            for col in range(8):
                print (' %3s' % quadrant.sector(7,col).glyph()),
            print '        SHIELDS         	%d' % (self.shields)
            print '---------------------------------------'

    def lrscan(self, gamestate):
        #"printLongRangeSensorScan"
        if self.damage('lrsensors') < 0:
            print "LONG RANGE SENSORS ARE INOPERABLE"
        else:
            print "LONG RANGE SENSOR SCAN FOR QUADRANT %d,%d" % (self.Q1,self.Q2)
            print "-------------------"
            for I in range(self.Q1-1, self.Q1+2):
                print ":",
                for J in range(self.Q2-1,self.Q2+2):
                    if I>=0 and I<=7 and J>=0 and J<=7:
                        quadrant = Galaxy.galaxy.quadrant(I,J)
                        if self.damage('computer')>=0: #"cumulative galaxy map is functioning"
                            self.computer_galaxy_scan.quadrants[I,J] = quadrant
                        print ("%d%d%d :" % (quadrant.num_klingons, quadrant.num_starbases, quadrant.num_stars)),
                    else:
                        print "000 :",
                print "\n-------------------"
        
    def devicename(self,n):
        "****  PRINTS DEVICE NAME FROM ARRAY *****"
        return self.system_names[n]

    def damage(self,s):
        return self.curdamage[s]

    #"ship.damage(): damage array.  Values < 0 indicate damage"
    def clear_damage(self):
        for s in self.systems:
            self.curdamage[s] = 0

    def printDamageControlReport(self, gamestate):
        if self.curdamage['damagectl'] < 0:
            print "DAMAGE CONTROL REPORT IS NOT AVAILABLE"
        else:
            print 
            print "DEVICE        STATE OF REPAIR\n"
            for R1 in self.systems:
                print '%-15s %d' % (self.devicename(R1), self.curdamage[R1])
            print 

    def checkForDamage(self, gamestate):
        if self.condition == 'DOCKED':
            print 'STAR BASE SHIELDS PROTECT THE ENTERPRISE'
        else:
            quadrant = Galaxy.galaxy.quadrant(self.Q1,self.Q2)
            ks = quadrant.klingons()
            for klingon in ks:
                if klingon.shields > 0:
                    H = (klingon.shields / SectorPos(self.pos.row,self.pos.col).distance_to(klingon.pos)) * uniform(0,2)
                    self.shields -= H
                    print '%4d UNIT HIT ON ENTERPRISE AT SECTOR %d,%d,   (%4d LEFT)' % (H, klingon.pos.row, klingon.pos.col, self.shields)
                    if self.shields<0:
                        gamestate.DESTROYED = True
                        return
                else:
                    pass # should be destroy_klingon()

    def move(self, gamestate, course, warp_factor):
        #"quadrant is 8 units across.  Warp factor is a fraction of that."
        N=int(warp_factor*8)

        #"erase our current position?"
        print 'Removing starship at [{0},{1}].sector({2},{3})'.format(self.Q1,self.Q2,self.pos.row, self.pos.col)
        Galaxy.galaxy.quadrants[self.Q1,self.Q2].setsector(Sector(self.pos.row, self.pos.col))

        # "save our starting position"
        X=self.pos.row
        Y=self.pos.col
        oldqrow = self.Q1
        oldqcol = self.Q2

        # "calculate the incremental delta movement"
        C2=int(course)
        X1=C[C2,1]+(C[C2+1,1]-C[C2,1])*(course-C2)
        X2=C[C2,2]+(C[C2+1,2]-C[C2,2])*(course-C2)

        #"move in N steps, checking at each step for collisions"
        for I in range(N):
            self.pos.row += X1
            self.pos.col += X2
            if self.pos.row< -.5 or self.pos.row >= 7.5 or self.pos.col<-.5 or self.pos.col >= 7.5:
                #"flew out of the quadrant."
                X=self.Q1*8+X+X1*N
                Y=self.Q2*8+Y+X2*N
                self.Q1=int(X/8)
                self.Q2=int(Y/8)
                self.pos.row=int(X-self.Q1*8+.5)
                self.pos.col=int(Y-self.Q2*8+.5)
                # check for wrap-around
                if self.pos.row < 0:
                    self.Q1=self.Q1-1
                    self.pos.row=7
                if self.pos.col < 0:
                    self.Q2=self.Q2-1
                    self.pos.col=7
                if self.pos.row > 7:
                    self.Q1 += 1
                    self.pos.row = 0
                if self.pos.col > 7:
                    self.Q2 += 1
                    self.pos.col = 0
                if self.Q1 < 0:
                    self.Q1 = 7
                if self.Q1 > 7:
                    self.Q1 = 0

                gamestate.T += 1 #"time passes"
                self.energy -= N+5
                gamestate.SAMEQUADRANT=False
                return
            
            occupant = Galaxy.galaxy.quadrant(self.Q1,self.Q2).sector(int(self.pos.row),int(self.pos.col))
            if occupant.type != Sector.SPACE:
                print " WARP ENGINES SHUTDOWN AT SECTOR %d,%d DUE TO BAD NAVIGATION" % (self.pos.row,self.pos.col)
                self.pos.row -= X1
                self.pos.col -= X2
                break

        # NOT a new quadrant...
        
        #"spend the energy it took to move here"
        self.energy = self.energy - N + self.shields
        if warp_factor>=1:
            gamestate.T += 1 #"time passes"

        #"no collisions -- set our new position"
        self.pos.row=int(self.pos.row)
        self.pos.col=int(self.pos.col)

        Galaxy.galaxy.quadrants[self.Q1,self.Q2].setsector(Starship(self.pos.row,self.pos.col))

        self.srscan(gamestate)

    def setCourse(self, gamestate):
        C1=0
        while C1<1 or C1>=9 or W1<0 or W1>8:
            print "COURSE (1-9):",
            try:
                C1 = input('? ')
            except:
                C1 = 0
            if C1==0:
                return
            if C1>=1 and C1 < 9:
                print "WARP FACTOR (0-8):",
                try:
                    W1 = input('? ')
                except:
                    W1 = 0
                if W1>=0 and W1<=8:
                    if self.damage('warpdrive')<0 and W1 > .2:
                        print "WARP ENGINES ARE DAMAGED, MAXIMUM SPEED = WARP .2"
                        W1=-1
        if Galaxy.galaxy.quadrant(self.Q1,self.Q2).num_klingons > 0:
            self.checkForDamage(gamestate)
        if gamestate.DESTROYED:
            return
        if self.energy<=0:
            if self.shields<1:
                print "THE ENTERPRISE IS DEAD IN SPACE. IF YOU SURVIVE ALL IMPENDING"
                print "ATTACK YOU WILL BE DEMOTED TO THE RANK OF PRIVATE"
                #"this is a death loop -- wait until either Enterprise is destroyed"
                #"or all Klingons in this quadrant self-destruct"
                while Galaxy.galaxy.quadrant(self.Q1,self.Q2).num_klingons>0:
                    self.checkForDamage(gamestate)
                print "THERE ARE STILL %d KLINGON BATTLE CRUISERS" % (Galaxy.galaxy.remaining_klingons)
                gamestate.RESTART=True
                return
            print "YOU HAVE %d UNITS OF ENERGY" % (self.energy)
            print "SUGGEST YOU GET SOME FROM YOUR SHIELDS WHICH HAVE UNITS LEFT" % (self.shields)
            return

        # repair a little damage?
        for I in self.systems:
            if self.curdamage[I] < 0:
                self.curdamage[I] += 1

        # cause a little damage?
        if randint(1,10) <= 2:
            R1 = choice(self.systems)
            print "\nDAMAGE CONTROL REPORT:",
            print self.devicename(R1),
            if randint(0,1) == 1:
                self.curdamage[R1] -= randint(1,5)
                print " DAMAGED"
            else:
                self.curdamage[R1] += randint(1,5)
                print " STATE OF REPAIR IMPROVED"
            print 

        self.move(gamestate, C1,W1)
    
    def firePhasers(self, gamestate):
        quadrant = Galaxy.galaxy.quadrant(self.Q1,self.Q2)
        if quadrant.num_klingons <= 0:
            print "SHORT RANGE SENSORS REPORT NO KLINGONS IN THIS QUANDRANT"
            return
        if self.damage('phasers') < 0:
            print "PHASER CONTROL IS DISABLED"
            return
        if self.damage('computer') < 0:
            print " COMPUTER FAILURE HAMPERS ACCURACY"
        X = 0
        while self.energy > X and X == 0:
            print "PHASERS LOCKED ON TARGET.  ENERGY AVAILABLE=%d" % (self.energy)
            print "NUMBER OF UNITS TO FIRE:";
            try:
                X = input('? ')
            except:
                X = -1
            if X <= 0:
                return
        self.energy -= X
        self.checkForDamage(gamestate)
        if gamestate.DESTROYED:
            return
        if self.damage('computer') < 0:
            X *= random()  #"computer failure result"
        for klingon in quadrant.klingons():
            if klingon.shields > 0:
                H=(X/quadrant.num_klingons/self.pos.distance_to(klingon.pos))*(2*random())
                klingon.shields -= H
                print "%4d UNIT HIT ON KLINGON AT SECTOR %d,%d   (%3d LEFT)" % (H, klingon.pos.row, klingon.pos.col, klingon.shields)
                if klingon.shields<0:
                    print "KLINGON AT SECTOR %d,%d DESTROYED ****" % (klingon.pos.row, klingon.pos.col)
                    quadrant.setsector(Sector(klingon.pos.row, klingon.pos.col))
                    Galaxy.galaxy.remaining_klingons -= 1
                    if Galaxy.galaxy.remaining_klingons <= 0:                    return
        if self.energy<0:
            print "OUT OF ENERGY"
            gamestate.DESTROYED=True

    def firePhotons(self, gamestate):
        if self.damage('photons') < 0:
            print "PHOTON TUBES ARE NOT OPERATIONAL"
            return
        if self.photons<=0:
            print "ALL PHOTON TORPEDOES EXPENDED"
            return
        C1=0
        while C1<1 or C1 >= 9:
            print "TORPEDO COURSE (1-9):",
            try:
                C1 = input('? ')
            except:
                C1 = 0
            if C1 == 0:
                return

        C2=int(C1)
        X1=C[C2,1]+(C[C2+1,1]-C[C2,1])*(C1-C2)
        X2=C[C2,2]+(C[C2+1,2]-C[C2,2])*(C1-C2)
        X=self.pos.row
        Y=self.pos.col
        self.photons -= 1
        quadrant = Galaxy.galaxy.quadrant(self.Q1,self.Q2)
        print "TORPEDO TRACK:"
        #"Loop until we hit something or torpedo leaves the quadrant"
        hit = False
        while not hit:
            X=X+X1
            Y=Y+X2
            Z3=1
            if X<-.5 or X >= 7.5 or Y<-.5 or Y >= 7.5:
                print "TORPEDO MISSED"
                break
            print "               {:.1f},{:.1f}".format(X,Y)
            if quadrant.sector(int(X+.5),int(Y+.5)).type != Sector.SPACE:
                hit = True

        X = int(X)
        Y = int(Y)
        sector = quadrant.sector(X,Y)
        if hit:
            if sector.type == Sector.STAR:
                print "YOU CAN'T DESTROY STARS SILLY"
                print "TORPEDO MISSED"
            elif sector.type == Sector.KLINGON:
                print "\07*** KLINGON DESTROYED ***"
                quadrant.setsector(Sector(X, Y))
                Galaxy.galaxy.remaining_klingons -= 1
                if Galaxy.galaxy.remaining_klingons <= 0:
                    return
            elif sector.type == Sector.STARBASE:
                print "\07*** STAR BASE DESTROYED ***  .......CONGRATULATIONS"
                quadrant.setsector(Sector(X, Y))
            else:
                print "HIT SOMETHING UNKNOWN"
                quadrant.setsector(Sector(X, Y))

        self.checkForDamage(gamestate)
        if self.energy < 0:
            print "OUT OF ENERGY"
            gamestate.DESTROYED=True

    def shieldControl(self, gamestate):
        if self.damage('computer') < 0:
            print "SHIELD CONTROL IS NON-OPERATIONAL"
        else:
            X = 0
            orig_energy = self.energy
            orig_shields = self.shields
            done = False
            while not done:
                print ("ENERGY AVAILABLE =%d   NUMBER OF UNITS TO SHIELDS:" % (self.energy + self.shields)),
                try:
                    X = input('? ')
                except:
                    X = -1
                if X <= 0:
                    self.energy = orig_energy
                    self.shields = orig_shields
                    return
                self.energy = self.energy + self.shields - X
                self.shields = X
                done = self.energy >= 0 and self.shields >= 0

    def computerControl(self, gamestate):
        if self.damage('computerctl') < 0:
            print "COMPUTER DISABLED"
            return
        while True:
            print "COMPUTER ACTIVE AND AWAITING COMMAND",
            try:
                A = input(': ')
            except:
                A = -1
            if A == 0:
                self.printGalacticRecord(gamestate)
                return
            if A == 1:
                self.printStatusReport(gamestate)
                return
            if A == 2:
                self.photonTorpedoData(gamestate)
                return
            print "\nFUNCTIONS AVAILABLE FROM COMPUTER\n"
      	print "   0 = CUMULATIVE GALATIC RECORD"
      	print "   1 = STATUS REPORT"
      	print "   2 = PHOTON TORPEDO DATA\n"

    def printGalacticRecord(self, gamestate):
        print "COMPUTER RECORD OF GALAXY FOR QUADRANT %d,%d" % (self.Q1,self.Q2)
        print "     0     1     2     3     4     5     6     7"
        print "   ----- ----- ----- ----- ----- ----- ----- -----"
        for I in range(8):
            print ('%d ' % (I)),
            for J in range(8):
                q = self.computer_galaxy_scan.quadrant(I,J)
                print (' %d%d%d ' % (q.num_klingons, q.num_starbases, q.num_stars)),
            print '\n   ----- ----- ----- ----- ----- ----- ----- -----'

    def printStatusReport(self, gamestate):
        print "\n   STATUS REPORT\n"
        print "NUMBER OF KLINGONS LEFT  = %d" % (Galaxy.galaxy.remaining_klingons)
        print "NUMBER OF STARDATES LEFT = %d" % (T0+MAX_STARDATES-gamestate.T)
        print "NUMBER OF STARBASES LEFT = %d" % (Galaxy.galaxy.remaining_starbases)
        self.printDamageControlReport(gamestate)

    def photonTorpedoData(self, gamestate):
        print 
        for klingon in Galaxy.galaxy.quadrant(self.Q1, self.Q2).klingons():
            if klingon.shields > 0:
                C1=self.pos.row
                A=self.pos.col
                W1=klingon.pos.row
                X=klingon.pos.row
                printDistanceAndDirection(C1,A,W1,X)

        while True:
            A=''
            while A[0] != 'Y' and A[0] != 'N':
                print "DO YOU WANT TO USE THE CALCULATOR",
                try:
                    A = raw_input('? ')
                except:
                    A = 'N'

            if A[0] == "N":
                return

            #"calculator"
            print "YOU ARE AT QUADRANT ( %d,%d )  SECTOR ( %d,%d )" % (self.Q1,self.Q2,self.pos.row,self.pos.col)
            print "SHIP'S & TARGET'S COORDINATES ARE",
            try:
                str = raw_input('? ')
                (C1,A,W1,X) = str.split()
                printDistanceAndDirection(C1,A,W1,X)
            except:
                print 'INPUT GARBLED'

# ===================================

def printDir(ship_r, ship_c, targ_r, targ_c):
    xdelta=targ_c-ship_c
    ydelta=ship_r-targ_r
    if xdelta>=0:
        if ydelta<0:
             if ABS(ydelta) < ABS(xdelta):
                 print "DIRECTION =%f" % (ship_r+(((ABS(xdelta)-ABS(ydelta))+ABS(xdelta))/ABS(xdelta)))
             else:
                 print "DIRECTION =%f" % (ship_r+(ABS(xdelta)/ABS(ydelta)))
             return (xdelta,ydelta)
        if xdelta<=0:
            if ydelta==0:
                ship_r=5
                if ABS(ydelta) <= ABS(xdelta):
                    print "DIRECTION =%f" % (ship_r+(ABS(ydelta)/ABS(xdelta)))
                else:
                    print "DIRECTION =%f" % (ship_r+(((ABS(ydelta)-ABS(xdelta))+ABS(ydelta))/ABS(ydelta)))
                return (xdelta,ydelta)
        ship_r=1
        if ABS(ydelta) > ABS(xdelta):
            print "DIRECTION =%f" % (ship_r+(((ABS(ydelta)-ABS(xdelta))+ABS(ydelta))/ABS(ydelta)))
            return (xdelta,ydelta)
        print "DIRECTION =%f" % (ship_r+(ABS(ydelta)/ABS(xdelta)))
        return (xdelta,ydelta)
    if ydelta>0:
        ship_r=3
        if ABS(ydelta) >= ABS(xdelta):
            print "DIRECTION =%f" % (ship_r+(ABS(xdelta)/ABS(ydelta)))
        else:
            print "DIRECTION =%f" % (ship_r+(((ABS(xdelta)-ABS(ydelta))+ABS(xdelta))/ABS(xdelta)))
        return (xdelta,ydelta)
    if xdelta != 0:
        ship_r=5
        if ABS(ydelta) <= ABS(xdelta):
            print "DIRECTION =%f" % (ship_r+(ABS(ydelta)/ABS(xdelta)))
        else:
            print "DIRECTION =%f" % (ship_r+(((ABS(ydelta)-ABS(xdelta))+ABS(ydelta))/ABS(ydelta)))
    else:
        ship_r=7
        if ABS(ydelta) < ABS(xdelta):
            print "DIRECTION =%f" % (ship_r+(((ABS(xdelta)-ABS(ydelta))+ABS(xdelta))/ABS(xdelta)))
        else:
            print "DIRECTION =%f" % (ship_r+(ABS(xdelta)/ABS(ydelta)))

def printDistanceAndDirection(ship_r, ship_c, targ_r, targ_c):
    [xdelta,ydelta] = printDir(ship_r, ship_c, targ_r, targ_c)
    print "DISTANCE =%f" % (sqrt(pow(xdelta,2)+pow(ydelta,2)))

# ===================================

def setCourse(gamestate, ship):
    ship.setCourse(gamestate)

def printShortRangeSensorScan(gamestate, ship):
    ship.srscan(gamestate)

def printLongRangeSensorScan(gamestate, ship):
    ship.lrscan(gamestate)

def firePhasers(gamestate, ship):
    ship.firePhasers(gamestate)

def firePhotons(gamestate, ship):
    ship.firePhotons(gamestate)

def shieldControl(gamestate, ship):
    ship.shieldControl(gamestate)

def printDamageControlReport(gamestate, ship):
    ship.printDamageControlReport(gamestate)

def computerControl(gamestate, ship):
    ship.computerControl(gamestate)

class game_session(object):
    "top level class for the program"

    commands = {
        '0' : setCourse,
        '1' : printShortRangeSensorScan,
        '2' : printLongRangeSensorScan,
        '3' : firePhasers,
        '4' : firePhotons,
        '5' : shieldControl,
        '6' : printDamageControlReport,
        '7' : computerControl
        }

    def __init__(self):
        self.GAMEOVER = False
        self.DESTROYED = False
        self.SAMEQUADRANT = False
        self.RESTART = False

    def showInstructions(self):
        print "\n     INSTRUCTIONS:\n\n"
        print "<*> = ENTERPRISE"
        print "+++ = KLINGON"
        print ">!< = STARBASE"
        print " *  = STAR"
        print "\n\n\nCOMMAND 0 = WARP ENGINE CONTROL"
        print "  'COURSE IS IN A CIRCULAR NUMERICAL           4  3  2"
        print "  VECTOR ARRANGEMENT AS SHOWN.                  \\ ^ /"
        print "  INTERGER AND REAL VALUES MAY BE                \\^/"
        print "  USED.  THEREFORE COURSE 1.5 IS              5 ------ 1"
        print "  HALF WAY BETWEEN 1 AND 2.                      /^\\"
        print "                                                / ^ \\"
        print "  A VECTOR OF 9 IS UNDEFINED, BUT              6  7  8"
        print "  VALUES MAY APPROACH 9."
        print "                                               COURSE"
        print "  ONE 'WARP FACTOR' IS THE SIZE OF"
        print "  ONE QUADRANT.  THEREFORE TO GET"
        print "  FROM QUADRANT 6,5 TO 5,5 YOU WOULD"
        print "  USE COURSE 3, WARP FACTOR 1"
        print "\n\nCOMMAND 1 = SHORT RANGE SENSOR SCAN"
        print "  PRINTS THE QUADRANT YOU ARE CURRENTLY IN, INCLUDING"
        print "  STARS, KLINGONS, STARBASES, AND THE ENTERPRISE; ALONG"
        print "  WITH OTHER PERTINATE INFORMATION."
        print "\n\nCOMMAND 2 = LONG RANGE SENSOR SCAN"
        print "  SHOWS CONDITIONS IN SPACE FOR ONE QUADRANT ON EACH SIDE"
        print "  OF THE ENTERPRISE IN THE MIDDLE OF THE SCAN.  THE SCAN"
        print "  IS CODED IN THE FORM XXX, WHERE THE UNITS DIGIT IS THE"
        print "  NUMBER OF STARS, THE TENS DIGIT IS THE NUMBER OF STAR-"
        print "  BASES, THE HUNDREDS DIGIT IS THE NUMBER OF KLINGONS."
        print "\n\nCOMMAND 3 = PHASER CONTROL"
        print "  ALLOWS YOU TO DESTROY THE KLINGONS BY HITTING HIM WITH"
        print "  SUITABLY LARGE NUMBERS OF ENERGY UNITS TO DEPLETE HIS "
        print "  SHIELD POWER.  KEEP IN MIND THAT WHEN YOU SHOOT AT"
        print "  HIM, HE GONNA DO IT TO YOU TOO."
        print "\n\nCOMMAND 4 = PHOTON TORPEDO CONTROL"
        print "  COURSE IS THE SAME AS USED IN WARP ENGINE CONTROL"
        print "  IF YOU HIT THE KLINGON, HE IS DESTROYED AND CANNOT FIRE"
        print "  BACK AT YOU.  IF YOU MISS, HE WILL SHOOT HIS PHASERS AT"
        print "  YOU."
        print "   NOTE: THE LIBRARY COMPUTER (COMMAND 7) HAS AN OPTION"
        print "   TO COMPUTE TORPEDO TRAJECTORY FOR YOU (OPTION 2)."
        print "\n\nCOMMAND 5 = SHIELD CONTROL"
        print "  DEFINES NUMBER OF ENERGY UNITS TO BE ASSIGNED TO SHIELDS"
        print "  ENERGY IS TAKEN FROM TOTAL SHIP'S ENERGY."
        print "\n\nCOMMAND 6 = DAMAGE CONTROL REPORT"
        print "  GIVES STATE OF REPAIRS OF ALL DEVICES.  A STATE OF REPAIR"
        print "  LESS THAN ZERO SHOWS THAT THAT DEVICE IS TEMPORARALY"
        print "  DAMAGED."
        print "\n\nCOMMAND 7 = LIBRARY COMPUTER"
        print "  THE LIBRARY COMPUTER CONTAINS THREE OPTIONS:"
        print "    OPTION 0 = CUMULATIVE GALACTIC RECORD"
        print "     SHOWS COMPUTER MEMORY OF THE RESULTS OF ALL PREVIOUS"
        print "     LONG RANGE SENSOR SCANS"
        print "    OPTION 1 = STATUS REPORT"
        print "     SHOWS NUMBER OF KLINGONS, STARDATESC AND STARBASES"
        print "     LEFT."
        print "    OPTION 2 = PHOTON TORPEDO DATA"
        print "     GIVES TRAJECTORY AND DISTANCE BETWEEN THE ENTERPRISE"
        print "     AND ALL KLINGONS IN YOUR QUADRANT\n\n\n\n\n"

    def main(self):
        for i in range(12):
            print
        print '                          STAR TREK '
        print "\n\n\nDO YOU WANT INSTRUCTIONS (THEY'RE LONG!)",
        try:
            answer = raw_input('? ')
        except:
            answer = ''
        if answer[0:3] == "YES":
            self.showInstructions()

        #*****  PROGRAM STARTS HERE *****
        print '\n\n\n\n\n\n\n\n\n\n\n\n'

        #"T0: stardate mission began"
        #"T: current stardate"
        self.T=randint(20,39)*100
        self.T0=self.T

        #"T7: the time at which the mission began"
        start_time = time()

        Galaxy.galaxy = Galaxy()

        enterprise = Ship()

        print "YOU MUST DESTROY %d KLINGONS IN %d STARDATES WITH %d STARBASES" % (Galaxy.galaxy.remaining_klingons, MAX_STARDATES, Galaxy.galaxy.remaining_starbases)

        self.RESTART=False
        while not self.GAMEOVER and not self.RESTART:
            # XXX - debug code
            for i in range(8):
                for j in range(8):
                    if Galaxy.galaxy.quadrant(enterprise.Q1,enterprise.Q2).sector(i,j).type == Sector.KLINGON:
                        print 'SECTOR(%d,%d)=KLINGON' % (i,j)
        
            print 'ENTERING QUADRANT %d,%d' % (enterprise.Q1,enterprise.Q2)
            q = Galaxy.galaxy.quadrant(enterprise.Q1,enterprise.Q2)
            q.setsector(Starship(enterprise.pos.row, enterprise.pos.col), adjust=True)
            if q.num_klingons > 0 and enterprise.shields<=200:
                print "COMBAT AREA      CONDITION RED"
                print "   SHIELDS DANGEROUSLY LOW"

            enterprise.srscan(self)

            self.DESTROYED=False
            self.SAMEQUADRANT=True
            while self.SAMEQUADRANT and not self.RESTART and not self.GAMEOVER:
                if Galaxy.galaxy.remaining_klingons<=0:
                    print 
                    print "THE LAST KLINGON BATTLE CRUISER IN THE GALAXY HAS BEEN DESTROYED"
                    print "THE FEDERATION HAS BEEN SAVED !!!"
                    print 
                    print "YOUR EFFICIENCY RATING ="((Galaxy.galaxy.total_klingons/(self.T-self.T0))*1000)
                    T1=time()
                    print "YOUR ACTUAL TIME OF MISSION = %d MINUTES" % (int((T1-Starimer)/60))
                    self.RESTART=True
                elif self.DESTROYED:
                    print
                    print "THE ENTERPRISE HAS BEEN DESTROYED. THE FEDERATION WILL BE CONQUERED"
                    self.GAMEOVER=True
                elif self.T>self.T0+MAX_STARDATES:
                    print
                    print "IT IS STARDATE %d" % (self.T)
                    print "MISSION TIME LIMIT EXPIRED.  YOU HAVE FAILED."
                    self.RESTART=True
                else:
                    print "COMMAND:",
                    try:
                        A = raw_input('? ')
                        if A[0] == '-':
                            return True
                        cmd = self.commands[A[0]]
                    except:
                        cmd = None
                    if cmd:
                        cmd(self, enterprise)
                    else:
                        print 
                        print "   0 = SET COURSE"
                        print "   1 = SHORT RANGE SENSOR SCAN"
                        print "   2 = LONG RANGE SENSOR SCAN"
                        print "   3 = FIRE PHASERS"
                        print "   4 = FIRE PHOTON TORPEDOES"
                        print "   5 = SHIELD CONTROL"
                        print "   6 = DAMAGE CONTROL REPORT"
                        print "   7 = CALL ON LIBRARY COMPUTER"
                        print "  -1 = QUIT"
                        print 

        if Galaxy.galaxy.remaining_klingons>0:
            print "THERE ARE STILL %d KLINGON BATTLE CRUISERS" % (Galaxy.galaxy.remaining_klingons)

if __name__ == '__main__':
    done = False
    while not done:
        game = game_session()
        done = game.main()
    quit()

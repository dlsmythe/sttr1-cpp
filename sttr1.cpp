// sttr1 - converted from a python port starting 2017-6-26

#include <iostream>
#include <iomanip>
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>
#include <cmath>
#include <random>
#include <chrono>
#include <bitset>
#include <cstdarg>

namespace dbg {

    enum class flag {
	verbose,
	sector,
	    quadrant,
	    galaxy,
	    init,
	    game,
	    
	    numflags
	    };
    using dbgflags = typename std::bitset<static_cast<int>(dbg::flag::numflags)>;
    dbgflags g_flags;
    int g_level;

    extern "C" {
	int vasprintf(char **strp, const char *fmt, va_list ap);
    }
    bool dbg_predicate(dbg::flag flag, int level) {
	return (g_flags[static_cast<int>(flag::verbose)] || 0 == g_level || (g_flags[static_cast<int>(flag)]  &&  g_level >= level));
    }
    void print(dbg::flag flag, int level, const char *filename, const char *funcname, int lineno, std::string fmt, ...) {
	if (dbg_predicate(flag, level)) {
	    va_list ap;
	    char *msg;

	    va_start(ap, fmt);
	    vasprintf(&msg, fmt.c_str(), ap);
	    std::cerr << filename << ":" << lineno << "[" << funcname << "] " << msg << std::endl;
	    va_end(ap);
	    free(msg);
	}
    }

#define DBGENABLE(F) dbg::g_flags[static_cast<int>(dbg::flag::F)] = true
#define DVERB(LVL,FMT,...) dbg::print(dbg::flag::verbose, (LVL), __FILE__, __FUNCTION__, __LINE__, FMT, ## __VA_ARGS__)
#define DGALAXY(LVL,FMT,...) dbg::print(dbg::flag::galaxy, (LVL), __FILE__, __FUNCTION__, __LINE__, FMT, ## __VA_ARGS__)
#define DQUAD(LVL,FMT,...) dbg::print(dbg::flag::quadrant, (LVL), __FILE__, __FUNCTION__, __LINE__, FMT, ## __VA_ARGS__)
#define DSECTOR(LVL,FMT,...) dbg::print(dbg::flag::sector, (LVL), __FILE__, __FUNCTION__, __LINE__, FMT, ## __VA_ARGS__)
#define DINIT(LVL,FMT,...) dbg::print(dbg::flag::init, (LVL), __FILE__, __FUNCTION__, __LINE__, FMT, ## __VA_ARGS__)
#define DGAME(LVL,FMT,...) dbg::print(dbg::flag::game, (LVL), __FILE__, __FUNCTION__, __LINE__, FMT, ## __VA_ARGS__)
#define DEXEC(LVL,FLG,...) do { if (dbg_predicate((FLG),(LVL))) { __VA_ARGS__ }}while (false)
}

namespace std {

    const int KLINGON_MAX_SHIELDS=200;

    // MAX_STARDATES: maximum number of stardates the mission contains
    const int MAX_STARDATES=30;

    //C[]: course array
    const int C[2][9] {
	{  0, -1, -1, -1,  0,  1,  1,  1,  0 },	// X
	    {  1,  1,  0, -1, -1, -1,  0,  1,  1 }	// Y
    };

    int randrange(int n) {
	return trunc(n * ((rand()-1) / (float)RAND_MAX));
    }
    
    class SectorPos {

    public:
	SectorPos(int r, int c): row_(r), col_(c) {}
	const SectorPos& operator=(const SectorPos& p) {
	    row_ = p.row_;
	    col_ = p.col_;
	    return *this;
	}
	
	int row() { return row_; }
	int col() { return col_; }
	
	float distance_to(SectorPos& pos) {
	    return sqrt(pow(pos.row() - row_,2.0)+pow(pos.col() - col_,2.0));
	}

    private:
	int row_;
	int col_;
    };

    class Sector {

    public:
	static int ID;
	
	enum class type {
	    SPACE,
		STAR,
		STARBASE,
		STARSHIP,
		KLINGON
		};

	Sector() = delete;
	Sector(int row, int col, Sector::type type = Sector::type::SPACE): pos_(SectorPos(row,col)), type_(type), id_(ID++) {
	    DSECTOR(2, "new space %d at sector %d,%d", id(), row, col);
	}
	Sector(const Sector& rhs): pos_(rhs.pos_), type_(rhs.type_), id_(ID++) {
	    DSECTOR(2, "copy-constructing sector with id %d to new one with id %d", rhs.id_, id_);
	}
	Sector(const Sector&& rhs): pos_(rhs.pos_), type_(rhs.type_), id_(ID++) {
	    DSECTOR(2, "move-constructing sector with id %d to new one with id %d", rhs.id_, id_);
	}
	Sector& operator=(const Sector& p) {
	    id_ = ID++;
	    pos_ = p.pos_;
	    type_ = p.type_;
	    DSECTOR(2, "copy-assigning sector with id %d to new one with id %d", p.id_, id_);
	    return *this;
	}
	Sector& operator=(const Sector&& p) {
	    id_ = ID++;
	    pos_ = p.pos_;
	    type_ = p.type_;
	    DSECTOR(2, "move-assigning sector with id %d to new one with id %d", p.id_, id_);
	    return *this;
	}
	virtual ~Sector() {
	    DSECTOR(2, "destructing sector with id %d", id_);
	}
	virtual string glyph() { return "   "; }
	Sector::type type() { return type_; }
	int row() { return pos_.row(); }
	int col() { return pos_.col(); }
	SectorPos& pos() { return pos_; }
	virtual int shields() { return 0; }
	virtual void shields(int val) { }
	int id() { return id_; }

    private:
	SectorPos pos_;
	enum type type_;
	int id_;
    };
    int Sector::ID = 0;
    
    class Klingon : public Sector {
    public:
	Klingon(int row, int col): Sector(row,col,Sector::type::KLINGON) {
	    DSECTOR(2, "new Klingon %d at sector %d,%d", id(), row, col);
	}
	Klingon(Klingon& rhs): Sector(rhs.row(),rhs.col(),Sector::type::KLINGON) {
	    shields_ = rhs.shields_;
	    DSECTOR(2, "copied Klingon %d at sector %d,%d", id(), row(), col());
	}
	virtual string glyph() override { return "+++"; }
	virtual int shields() override { return shields_; }
	virtual void shields(int val) override { shields_ = val; }
    private:
	int shields_ = 0;
    };
	
    class Starship : public Sector {
    public:
	Starship(int row, int col): Sector(row,col,Sector::type::STARSHIP) {
	    DSECTOR(2, "new Starship %d at sector %d,%d", id(), row, col);
	}
	virtual string glyph() override { return "<*>"; }
    };
	
    class Starbase : public Sector {
    public:
	Starbase(int row, int col): Sector(row,col,Sector::type::STARBASE) {
	    DSECTOR(2, "new Starbase %d at sector %d,%d", id(), row, col);
	}
	virtual string glyph() override { return ">!<"; }
    };
	
    class Star : public Sector {
    public:
	Star(int row, int col): Sector(row,col,Sector::type::STAR) {
	    DSECTOR(2, "new Star %d at sector %d,%d", id(), row, col);
	}
	virtual string glyph() override { return " * "; }
    };

    class Quadrant {
	// a quadrant is a 2D (row,col) array of Sector() pointers.
	// NB: can't be a value, since the objects are polymorphic

    public:
	Quadrant();
	void repopulate();
	pair<int,int> randomemptypos();
	Sector& sector(int row, int col);
	void setpos(int row, int col) {
	    g_row_ = row; g_col_ = col;
	    dump();
	}
	void setsector(Sector sec, bool adjust=false);
	int num_klingons() { return num_klingons_; }
	int num_starbases() { return num_starbases_; }
	int num_stars() { return num_stars_; }
	void dump();
	virtual ~Quadrant() {
	}

	vector<Klingon*> klingons() {
	    vector<Klingon*>kv {};
	    for (int row = 0; row < 8; row++) {
		for (int col = 0; col < 8; col++) {
		    if (sectors_[row][col]->type() == Sector::type::KLINGON) {
			kv.push_back(static_cast<Klingon*>(sectors_[row][col].get()));
		    }
		}
	    }
	    return kv;
	}

    private:
	array<array<unique_ptr<Sector>,8>,8> sectors_;
	int g_row_ {-1};
	int g_col_ {-1};
	int num_klingons_ {0};
	int num_starbases_ {0};
	int num_stars_ {0};

	pair<int,int> emptypos();
	pair<int,int> nearestempty(SectorPos& shippos);
    };

    Quadrant::Quadrant() {

	DQUAD(2, "INITIALIZING QUADRANT %d,%d", g_row_, g_col_);

	repopulate();
    }

    void Quadrant::repopulate() {
	for (int row = 0; row < 8; row++) {
	    for (int col = 0; col < 8; col++) {
		if (nullptr == sectors_[row][col]) {
		    sectors_[row][col] = make_unique<Sector>(row, col);
		}
	    }
	}

	DQUAD(2, "POPULATING QUADRANT %d,%d", g_row_, g_col_);

	//select # klingons for this quadrant, and add to total
	int R1 = randrange(100);
	if (R1>98) {
	    num_klingons_ = 3;
	} else if (R1>95) {
	    num_klingons_ = 2;
	} else if (R1>80) {
	    num_klingons_ = 1;
	} else {
	    num_klingons_ = 0;
	}
	DQUAD(2, "%d klingons", num_klingons_);

	int r, c;
	for (int i = 0; i < num_klingons_; i++) {
	    tie(r,c) = randomemptypos();
	    sectors_[r][c] = make_unique<Klingon>(r,c);
	    DQUAD(2, "new klingon: sectors_[%d][%d] = %d", r, c,sectors_[r][c]->id());
	}
                    
	//select # starbases for this quadrant, and add to total
	num_starbases_ = randrange(100) > 96 ? 1 : 0;
	for (int i = 0; i < num_starbases_; i++) {
	    tie(r,c) = randomemptypos();
	    sectors_[r][c] = make_unique<Starbase>(r,c);
	    DQUAD(2, "new Starbase: sectors_[%d][%d] = %d", r, c,sectors_[r][c]->id());
	}
	DQUAD(2, "%d starbases", num_starbases_);

	//select # of stars for this quadrant
	num_stars_ = 1 + randrange(8);
	for (int i = 0; i < num_stars_; i++) {
	    tie(r,c) = randomemptypos();
	    sectors_[r][c] = make_unique<Star>(r,c);
	    DQUAD(2, "new Star: sectors_[%d][%d] = %d", r, c,sectors_[r][c]->id());
	}
	DQUAD(2, "%d stars", num_stars_);
    }

    void Quadrant::dump() {
	DEXEC(2, dbg::flag::quadrant,
	cerr << "QUADRANT " << g_row_ << "," << g_col_
	     << " now looks like this:" << endl;
	for (int row = 0; row < 8; row++) {
	    cerr << row << " |";
	    for (int col = 0; col < 8; col++) {
		cerr << " " << setw(4) << (int)sectors_[row][col]->id() << " |";
	    }
	    cerr << endl;
	});
    }
    
    pair<int,int> Quadrant::emptypos() {
	for (int row = 0; row < 8; row++) {
	    for (int col = 0; col < 8; col++) {
		if (sectors_[row][col]->type() == Sector::type::SPACE) {
		    return pair<int,int>(row,col);
		}
	    }
	}
	throw logic_error("no space?");
    }

    pair<int,int> Quadrant::randomemptypos() {
	while (true) {
	    int r=randrange(8);
	    int c=randrange(8);
	    if (sectors_[r][c]->type() == Sector::type::SPACE) {
		return pair<int,int> {r,c};
	    }
	}
    }

    pair<int,int> Quadrant::nearestempty(SectorPos& shippos) {
	float mindist = (8*8) + (8*8); // actually, sqrt() of that, but this works.
	pair<int,int> minp {-1,-1};
	for (int row = 0; row < 8; row++) {
	    for (int col = 0; col < 8; col++) {
		if (sectors_[row][col]->type() != Sector::type::SPACE) {
		    continue;
		}
		float d = shippos.distance_to(sectors_[row][col]->pos());
		if (d < mindist) {
		    mindist = d;
		    minp = pair<int,int>(row,col);
		}
	    }
	}
	return minp;
    }

    Sector& Quadrant::sector(int row, int col) {
	if (row >= 0 && row < 8 && col >= 0 && col < 8) {
	    return *sectors_[row][col];
	}
	throw out_of_range("sector addr out of range");
    }

    void Quadrant::setsector(Sector sec, bool adjust) {
	Sector& cur = *sectors_[sec.row()][sec.col()];
	switch (sec.type()) {
	case Sector::type::STARSHIP:
	    if (cur.type() != Sector::type::SPACE) {
		if (!adjust) {
		    cout << "Landed on occupied space. Enterprise destroyed." << endl;
		    exit(0);
		}
	    }
	    int erow, ecol;
	    tie(erow, ecol) = nearestempty(sec.pos());
	    DQUAD(1, "landed on something - moving to nearby empty position");
	    sec.pos() = SectorPos(erow, ecol);
	    DQUAD(1, "Placing STARSHIP at sector position %d,%d", sec.row(), sec.col());
	    for (int row = 0; row < 8; row++) {
		for (int col = 0; col < 8; col++) {
		    Sector& s = *sectors_[row][col];
		    if (s.type() == Sector::type::KLINGON) {
			s.shields(KLINGON_MAX_SHIELDS);
			cout << "klingon at " << row << "," << col << " in quadrant " << g_row_ << "," << g_col_ << endl;
		    }
		    else if (s.type() == Sector::type::STARSHIP) {
			cout << "starship already in sector at position " << g_row_ << "," << g_col_ << endl;
		    }
		}
	    }
	    break;
	case Sector::type::KLINGON:
	    if (cur.type() == Sector::type::SPACE) {
		num_klingons_ += 1;
	    }
	    break;
	case Sector::type::SPACE:
	    if (cur.type() == Sector::type::KLINGON) {
		num_klingons_ -= 1;
	    }
	    else if (cur.type() == Sector::type::STARBASE) {
		num_starbases_ -= 1;
	    }
	    break;
	default:
	    break;
	}
	DSECTOR(2, "setting quadrant(%d,%d).sectors_[%d][%d] from id %d to id %d",
		g_row_, g_col_, sec.row(), sec.col(), cur.id(), sec.id());
	sectors_[sec.row()][sec.col()] = make_unique<Sector>(sec);
    }
		    
    class Galaxy {
	// the galaxy is a 2D (row,col) array of Quadrant objects.
	// There is only one Galaxy - accessed via an internal singleton.
	// The Ship's Computer has a copy of info from each quadrant it knows about,
	//  if the computer isn't damaged.
    public:    
	Galaxy(): total_klingons_(0), remaining_klingons_(0), remaining_starbases_(0) {
	    DGALAXY(1, "INITIALIZING GALAXY");
	    for (int row = 0; row < 8; row++) {
		for (int col = 0; col < 8; col++) {
		    galaxy_[row][col].setpos(row, col);
		    remaining_klingons_ +=  galaxy_[row][col].num_klingons();
		    remaining_starbases_ += galaxy_[row][col].num_starbases();
		}
	    }

	    DGALAXY(2, "allocating starbases and klingons");
	    // initialize the galaxy
	    // have to have at least 1 each of klingons and starbases
	    while (remaining_starbases_ <= 0 || remaining_klingons_ <= 0) {
		DGALAXY(2, "sb: %d kl: %d", remaining_starbases_, remaining_klingons_);
		int row = randrange(8);
		int col = randrange(8);

		if (galaxy_[row][col].num_klingons() == 0 && galaxy_[row][col].num_starbases() == 0) {
		    remaining_klingons_ -=  galaxy_[row][col].num_klingons();
		    remaining_starbases_ -= galaxy_[row][col].num_starbases();
		    galaxy_[row][col].repopulate();
		    remaining_klingons_ +=  galaxy_[row][col].num_klingons();
		    remaining_starbases_ += galaxy_[row][col].num_starbases();
		}
	    }

	    // save the original total number of klingons for the efficiency rating at the end of the game
	    total_klingons_ = remaining_klingons_;
	    DGALAXY(2, "Done.");
	}
	virtual ~Galaxy() {
	}

	static void reset() {
	    if (nullptr != singleton) {
		singleton = nullptr;
	    }
	}

	static pair<int,int> randomemptypos(int qrow, int qcol) {
	    if (nullptr == singleton) {
		singleton = unique_ptr<Galaxy>(new Galaxy());
	    }
	    return singleton->galaxy_[qrow][qcol].randomemptypos();
	}
    
	static Quadrant& quadrant(int row, int col) {
	    if (nullptr == singleton) {
		singleton = unique_ptr<Galaxy>(new Galaxy());
	    }
	    return singleton->galaxy_[row][col];
	}

	static int total_klingons() {
	    if (nullptr == singleton) {
		singleton = unique_ptr<Galaxy>(new Galaxy());
	    }
	    return singleton->total_klingons_;
	}
	static int remaining_klingons() {
	    if (nullptr == singleton) {
		singleton = unique_ptr<Galaxy>(new Galaxy());
	    }
	    return singleton->remaining_klingons_;
	}
	static int remaining_starbases() {
	    if (nullptr == singleton) {
		singleton = unique_ptr<Galaxy>(new Galaxy());
	    }
	    return singleton->remaining_starbases_;
	}

    private:
	array<array<Quadrant,8>,8> galaxy_ {};
	int total_klingons_;
	int remaining_klingons_;
	int remaining_starbases_;

	static unique_ptr<Galaxy>singleton;
    };

    unique_ptr<Galaxy> Galaxy::singleton = nullptr;

    class game_session {
	// top level class for the program

    public:
	game_session(): T_(0), T0_(0), start_time_(0) { Galaxy::reset(); }
	void showInstructions();
	bool mainloop();
	int stardate() { return T_; }
	void gameover() { GAMEOVER = true; }
	void destroyed() { DESTROYED = true; }

    private:
	bool GAMEOVER = false;
	bool DESTROYED = false;
	bool SAMEQUADRANT = false;
	bool RESTART = false;
	int T_; // current stardate
	int T0_; // starting stardate
	time_t start_time_;
    };

    class Ship {

    public:

	unordered_map<string,string> system_names {
	    { "warpdrive", "WARP ENGINES" },
		{ "srsensors", "S.R. SENSORS" },
		    { "lrsensors", "L.R. SENSORS" },
			{ "phasers"  , "PHASER CNTRL" },
			    { "photons"  , "PHOTON TUBES" },
				{ "damagectl", "DAMAGE CNTRL" },
				    { "shields"  , "SHIELD CNTRL" },
					{ "computer" , "COMPUTER" },
					    { "computerctl" , "COMPT. CNTRL" }
	};

	//MAXPHOTONS: full magazine of torpedos
	const int MAXPHOTONS = 10;

	//MAXENERGY: full charge of energy
	const int MAXENERGY = 3000;

	enum class cond {GREEN, YELLOW, RED };

	Ship(): DOCKED(false), photons(MAXPHOTONS), energy_(MAXENERGY), shields_(0), condition(Ship::cond::GREEN) {

	    clear_damage();
	    //computer_galaxy_scan = Galaxy();

	    //Q1 and Q2: X and Y coordinates of the current quadrant, 0..7 each
	    Q1=randrange(8);
	    Q2=randrange(8);

	    //S1 and S2, X and Y coordinates of the ship in the current quadrant, 1..8 each
	    int r, c;
	    tie(r, c) = Galaxy::randomemptypos(Q1,Q2);
	    pos_ = SectorPos(r,c);
	}

	int q_row() { return Q1; }
	int q_col() { return Q2; }
	int s_row() { return pos_.row(); }
	int s_col() { return pos_.col(); }
	void pos(int r, int c) { pos_ = SectorPos(r,c); }
	int shields() { return shields_; }

	void setCourse(game_session *gamestate);
	void srscan(game_session *gamestate);
	void lrscan(game_session *gamestate);
	void firePhasers(game_session *gamestate);
	void firePhotons(game_session *gamestate);
	void printDamageControlReport(game_session *gamestate);
	void shieldControl(game_session *gamestate);
	void computerControl(game_session *gamestate);
	
    private:
	bool DOCKED;
	map<string, int> curdamage_;
	int photons;
	int energy_;
	int shields_;
	SectorPos pos_ = SectorPos(0,0);
	int Q1, Q2; // ship's quadrant (r,c)
	enum Ship::cond condition;
	//Galaxy computer_galaxy_scan;

	void clear_damage(void);
	int damage(string sysname) { return curdamage_[sysname]; }
	string devicename(string sysname) { return system_names[sysname]; }
	void checkForDamage(game_session *gamestate);
	void move(game_session *gamestate, int course, int warp_factor);
	void printGalacticRecord(game_session *gamestate);
	void printStatusReport(game_session *gamestate);
	void photonTorpedoData(game_session *gamestate);

	string condname(cond c) {
	    if (cond::GREEN == c)
		return "GREEN";
	    if (cond::YELLOW == c)
		return "YELLOW";
	    if (cond::RED == c)
		return "RED";
	    return "XXX";
	}
    };

    void Ship::move(game_session *gamestate, int course, int warp_factor) {
    }
    void Ship::setCourse(game_session *gamestate) {
    }
    void Ship::firePhasers(game_session *gamestate) {
    }
    void Ship::firePhotons(game_session *gamestate) {
    }
    void Ship::computerControl(game_session *gamestate) {
    }
    void Ship::printGalacticRecord(game_session *gamestate) {
    }
    void Ship::printStatusReport(game_session *gamestate) {
    }
    void Ship::photonTorpedoData(game_session *gamestate) {
    }

    void Ship::srscan(game_session *gamestate) {
	// short-range sensor scan
	if (damage("srsensors") < 0) {
	    cout << "SR SENSORS INOPERATIVE" << endl;
	    return;
	}

	DGAME(1, "quadrant(%d,%d)", q_row(), q_col());
	DGAME(1, "ship sector(%d,%d)", s_row(), s_col());
	Quadrant& quadrant = Galaxy::quadrant(q_row(),q_col());
		
	// XXX debug
	for (int r = 0; r < 8; r++) {
	    for (int c = 0; c < 8; c++) {
		if (quadrant.sector(r,c).type() == Sector::type::STARSHIP) {
		    DGAME(1, "starship at %d,%d", r, c);
		}
	    }
	}

	// see if we're docked
	DOCKED=false;
	for (int I = s_row()-1; I <= s_row()+1 && !DOCKED; I++) {
	    for (int J = s_col()-1; J <= s_col()+1 && !DOCKED; J++) {
		if (I>=0 && I<=7 && J>=0 && J<=7) {
		    auto s = quadrant.sector(s_row(), s_col());
		    if (s.type() == Sector::type::STARBASE) {
			energy_ = MAXENERGY;
			photons = MAXPHOTONS;
			clear_damage();
			cout << "SHIELDS DROPPED FOR DOCKING PURPOSES" << endl;
			shields_ = 0;
			DOCKED=true;
		    }
		}
	    }
	}

	//selectCondition
	if (DOCKED)
	    condition=cond::GREEN;
	else if (quadrant.num_klingons()>0)
	    condition=cond::RED;
	else if (energy_ < MAXENERGY*.1)
	    condition=cond::YELLOW;
	else
	    condition=cond::GREEN;

	//printQuadrant
	cout << "---------------------------------------" << endl;
	for (int col = 0; col < 8; col++) {
	    cout << setw(3) << quadrant.sector(0,col).glyph();
	}
	cout << endl;
	for (int col = 0; col < 8; col++) {
	    cout << setw(3) << quadrant.sector(1,col).glyph();
	}
	cout << "        STARDATE        	" << gamestate->stardate() << endl;
	for (int col = 0; col < 8; col++) {
	    cout << setw(3) << quadrant.sector(2,col).glyph();
	}
	cout << "        CONDITION       	" << condname(condition) << endl;
	for (int col = 0; col < 8; col++) {
	    cout << setw(3) << quadrant.sector(3,col).glyph();
	}
	cout << "        QUADRANT        	" << q_row() << "," << q_col() << endl;
	for (int col = 0; col < 8; col++) {
	    cout << setw(3) << quadrant.sector(4,col).glyph();
	}
	cout << "        SECTOR          	" << s_row() << "," << s_col() << endl;
	for (int col = 0; col < 8; col++) {
	    cout << setw(3) << quadrant.sector(5,col).glyph();
	}
	cout << "        ENERGY          	" << energy_ << endl;
	for (int col = 0; col < 8; col++) {
	    cout << setw(3) << quadrant.sector(6,col).glyph();
	}
	cout << "        PHOTON TORPEDOES        " << photons << endl;
	for (int col = 0; col < 8; col++) {
	    cout << setw(3) << quadrant.sector(7,col).glyph();
	}
	cout << "        SHIELDS         	" << shields_ << endl;
	cout << "---------------------------------------" << endl;
    }

    
    void Ship::lrscan(game_session *gamestate) {
	//printLongRangeSensorScan
	if (damage("lrsensors") < 0) {
	    cout << "LONG RANGE SENSORS ARE INOPERABLE" << endl;
	} else {
	    cout << "LONG RANGE SENSOR SCAN FOR QUADRANT " << q_row() << "," << q_col() << endl;
	    cout << "-------------------" << endl;
	    for (int I = q_row()-1; I < q_row()+2; I++) {
		cout << ":";
		for (int J = q_col()-1; J < q_col()+2; J++) {
		    if (I>=0 && I<=7 && J>=0 && J<=7) {
			Quadrant& quadrant = Galaxy::quadrant(I,J);
			if (damage("computer")>=0) {  // cumulative galaxy map is functioning
			    //self.computer_galaxy_scan.quadrants[I,J] = quadrant;
			}
			cout << " "
			     << setw(1) << quadrant.num_klingons()
			     << setw(1) << quadrant.num_starbases()
			     << setw(1) << quadrant.num_stars()
			     << " :";
		    } else {
			cout << "000 :";
		    }
		}
		cout << endl << "-------------------" << endl;
	    }
	}
    }

    //"ship.damage(): damage array.  Values < 0 indicate damage"
    void Ship::clear_damage(void) {
	for (auto s : system_names) {
	    curdamage_[s.first] = 0;
	}
    }

    void Ship::printDamageControlReport(game_session *gamestate) {
	if (curdamage_["damagectl"] < 0) {
	    cout << "DAMAGE CONTROL REPORT IS NOT AVAILABLE" << endl;
	} else {
	    cout << endl << "DEVICE        STATE OF REPAIR" << endl;
	    for (auto s : curdamage_) {
		cout << " " << setw(15) << left << system_names[s.first] << " " << s.second << endl;
	    }
	    cout << endl;
	}
    }

    void Ship::checkForDamage(game_session *gamestate) {
	if (DOCKED) {
	    cout << "STAR BASE SHIELDS PROTECT THE ENTERPRISE" << endl;
	} else {
	    for (auto klingon : Galaxy::quadrant(Q1,Q2).klingons()) {
		if (klingon->shields() > 0) {
		    int H = (klingon->shields() / pos_.distance_to(klingon->pos())) * randrange(2);
		    shields_ -= H;
		    cout << setw(4) << H << " UNIT HIT ON ENTERPRISE AT SECTOR "<< klingon->row() << "," << klingon->col()
			 << "   (" << setw(4) << shields_ << " LEFT)" << endl;
		    if (shields_ < 0) {
			gamestate->destroyed();
			return;
		    }
		}
	    }
	}
    }

	//     def move(self, gamestate, course, warp_factor):
	//         //"quadrant is 8 units across.  Warp factor is a fraction of that."
	//         N=int(warp_factor*8)

	//         //"erase our current position?"
	//         print "Removing starship at [{0},{1}]->sector({2},{3})".format(self.Q1,self.Q2,self.pos.row, self.pos.col)
	//         Galaxy::galaxy.quadrants[self.Q1,self.Q2]->setsector(Sector(self.pos.row, self.pos.col))

	//         // "save our starting position"
	//         X=self.pos.row
	//         Y=self.pos.col
	//         oldqrow = self.Q1
	//         oldqcol = self.Q2

	//         // "calculate the incremental delta movement"
	//         C2=int(course)
	//         X1=C[C2-1,0]+(C[C2,0]-C[C2-1,0])*(course-(C2-1))
	//         X2=C[C2-1,1]+(C[C2,1]-C[C2-1,1])*(course-(C2-1))

	//         //"move in N steps, checking at each step for collisions"
	//         for I in range(N):
	//             self.pos.row += X1
	//             self.pos.col += X2
	//             if self.pos.row< -.5 or self.pos.row >= 7.5 or self.pos.col<-.5 or self.pos.col >= 7.5:
	//                 //"flew out of the quadrant."
	//                 X=self.Q1*8+X+X1*N
	//                 Y=self.Q2*8+Y+X2*N
	//                 self.Q1=int(X/8)
	//                 self.Q2=int(Y/8)
	//                 self.pos.row=int(X-self.Q1*8+.5)
	//                 self.pos.col=int(Y-self.Q2*8+.5)
	//                 // check for wrap-around
	//                 if self.pos.row < 0:
	//                     self.Q1=self.Q1-1
	//                     self.pos.row=7
	//                 if self.pos.col < 0:
	//                     self.Q2=self.Q2-1
	//                     self.pos.col=7
	//                 if self.pos.row > 7:
	//                     self.Q1 += 1
	//                     self.pos.row = 0
	//                 if self.pos.col > 7:
	//                     self.Q2 += 1
	//                     self.pos.col = 0
	//                 if self.Q1 < 0:
	//                     self.Q1 = 7
	//                 if self.Q1 > 7:
	//                     self.Q1 = 0

	//                 gamestate.stardate += 1 #"time passes"
	//                 self.energy -= N+5
	//                 gamestate.SAMEQUADRANT=False
	//                 return
            
	//             occupant = Galaxy::galaxy.quadrant(self.Q1,self.Q2).sector(int(self.pos.row),int(self.pos.col))
	//             if occupant.type != Sector.SPACE:
	//                 print " WARP ENGINES SHUTDOWN AT SECTOR %d,%d DUE TO BAD NAVIGATION" % (self.pos.row,self.pos.col)
	//                 self.pos.row -= X1
	//                 self.pos.col -= X2
	//                 break

	//         // NOT a new quadrant...
        
	//         //"spend the energy it took to move here"
	//         self.energy = self.energy - N + self.shields_
	//         if warp_factor>=1:
	//             gamestate.T += 1 #"time passes"

	//         //"no collisions -- set our new position"
	//         self.pos.row=int(self.pos.row)
	//         self.pos.col=int(self.pos.col)

	//         Galaxy::galaxy.quadrants[self.Q1,self.Q2]->setsector(Starship(self.pos.row,self.pos.col))

	//         self.srscan(gamestate)

	//     def setCourse(self, gamestate):
	//         C1=0
	//         while C1<1 or C1>=9 or W1<0 or W1>8:
	//             print "COURSE (1-9):",
	//             try:
	//                 C1 = input("? ")
	//             except:
	//                 C1 = 0
	//             if C1==0:
	//                 return
	//             if C1>=1 and C1 < 9:
	//                 print "WARP FACTOR (0-8):",
	//                 try:
	//                     W1 = input("? ")
	//                 except:
	//                     W1 = 0
	//                 if W1>=0 and W1<=8:
	//                     if self.damage("warpdrive")<0 and W1 > .2:
	//                         print "WARP ENGINES ARE DAMAGED, MAXIMUM SPEED = WARP .2"
	//                         W1=-1
	//         if Galaxy::galaxy.quadrant(self.Q1,self.Q2).num_klingons > 0:
	//             self.checkForDamage(gamestate)
	//         if gamestate.DESTROYED:
	//             return
	//         if self.energy<=0:
	//             if self.shields_<1:
	//                 print "THE ENTERPRISE IS DEAD IN SPACE. IF YOU SURVIVE ALL IMPENDING"
	//                 print "ATTACK YOU WILL BE DEMOTED TO THE RANK OF PRIVATE"
	//                 //"this is a death loop -- wait until either Enterprise is destroyed"
	//                 //"or all Klingons in this quadrant self-destruct"
	//                 while Galaxy::galaxy.quadrant(self.Q1,self.Q2).num_klingons>0:
	//                     self.checkForDamage(gamestate)
	//                 print "THERE ARE STILL %d KLINGON BATTLE CRUISERS" % (Galaxy::galaxy.remaining_klingons)
	//                 gamestate.RESTART=True
	//                 return
	//             print "YOU HAVE %d UNITS OF ENERGY" % (self.energy)
	//             print "SUGGEST YOU GET SOME FROM YOUR SHIELDS WHICH HAVE UNITS LEFT" % (self.shields_)
	//             return

	//         // repair a little damage?
	//         for I in self.systems:
	//             if self.curdamage[I] < 0:
	//                 self.curdamage[I] += 1

	//         // cause a little damage?
	//         if randint(1,10) <= 2:
	//             R1 = choice(self.systems)
	//             print "\nDAMAGE CONTROL REPORT:",
	//             print self.devicename(R1),
	//             if randint(0,1) == 1:
	//                 self.curdamage[R1] -= randint(1,5)
	//                 print " DAMAGED"
	//             else:
	//                 self.curdamage[R1] += randint(1,5)
	//                 print " STATE OF REPAIR IMPROVED"
	//             print 

	//         self.move(gamestate, C1,W1)
    
	//     def firePhasers(self, gamestate):
	//         quadrant = Galaxy::galaxy.quadrant(self.Q1,self.Q2)
	//         if quadrant.num_klingons <= 0:
	//             print "SHORT RANGE SENSORS REPORT NO KLINGONS IN THIS QUANDRANT"
	//             return
	//         if self.damage("phasers") < 0:
	//             print "PHASER CONTROL IS DISABLED"
	//             return
	//         if self.damage("computer") < 0:
	//             print " COMPUTER FAILURE HAMPERS ACCURACY"
	//         X = 0
	//         while self.energy > X and X == 0:
	//             print "PHASERS LOCKED ON TARGET.  ENERGY AVAILABLE=%d" % (self.energy)
	//             print "NUMBER OF UNITS TO FIRE:";
	//             try:
	//                 X = input("? ")
	//             except:
	//                 X = -1
	//             if X <= 0:
	//                 return
	//         self.energy -= X
	//         self.checkForDamage(gamestate)
	//         if gamestate.DESTROYED:
	//             return
	//         if self.damage("computer") < 0:
	//             X *= random()  #"computer failure result"
	//         for klingon in quadrant.klingons():
	//             if klingon.shields_ > 0:
	//                 H=(X/quadrant.num_klingons/self.pos.distance_to(klingon.pos))*(2*random())
	//                 klingon.shields -= H
	//                 print "%4d UNIT HIT ON KLINGON AT SECTOR %d,%d   (%3d LEFT)" % (H, klingon.pos.row, klingon.pos.col, klingon.shields)
	//                 if klingon.shields<0:
	//                     print "KLINGON AT SECTOR %d,%d DESTROYED ****" % (klingon.pos.row, klingon.pos.col)
	//                     quadrant.setsector(Sector(klingon.pos.row, klingon.pos.col))
	//                     Galaxy::galaxy.remaining_klingons -= 1
	//                     if Galaxy::galaxy.remaining_klingons <= 0:                    return
	//         if self.energy<0:
	//             print "OUT OF ENERGY"
	//             gamestate.DESTROYED=True

	//     def firePhotons(self, gamestate):
	//         if self.damage("photons") < 0:
	//             print "PHOTON TUBES ARE NOT OPERATIONAL"
	//             return
	//         if self.photons<=0:
	//             print "ALL PHOTON TORPEDOES EXPENDED"
	//             return
	//         C1=0
	//         while C1<1 or C1 >= 9:
	//             print "TORPEDO COURSE (1-9):",
	//             try:
	//                 C1 = input("? ")
	//             except:
	//                 C1 = 0
	//             if C1 == 0:
	//                 return

	//         C2=int(C1)
	//         X1=C[C2-1,0]+(C[C2,0]-C[C2-1,1])*((C1-1)-(C2-1))
	//         X2=C[C2-1,1]+(C[C2,1]-C[C2-1,2])*((C1-1)-(C2-1))
	//         X=self.pos.row
	//         Y=self.pos.col
	//         self.photons -= 1
	//         quadrant = Galaxy::galaxy.quadrant(self.Q1,self.Q2)
	//         print "TORPEDO TRACK:"
	//         //"Loop until we hit something or torpedo leaves the quadrant"
	//         hit = False
	//         while not hit:
	//             X=X+X1
	//             Y=Y+X2
	//             Z3=1
	//             if X<-.5 or X >= 7.5 or Y<-.5 or Y >= 7.5:
	//                 print "TORPEDO MISSED"
	//                 break
	//             print "               {:.1f},{:.1f}".format(X,Y)
	//             if quadrant.sector(int(X+.5),int(Y+.5)).type != Sector.SPACE:
	//                 hit = True

	//         X = int(X)
	//         Y = int(Y)
	//         sector = quadrant.sector(X,Y)
	//         if hit:
	//             if sector.type == Sector.STAR:
	//                 print "YOU CAN'T DESTROY STARS SILLY"
	//                 print "TORPEDO MISSED"
	//             elif sector.type == Sector.KLINGON:
	//                 print "\07*** KLINGON DESTROYED ***"
	//                 quadrant.setsector(Sector(X, Y))
	//                 Galaxy::galaxy.remaining_klingons -= 1
	//                 if Galaxy::galaxy.remaining_klingons <= 0:
	//                     return
	//             elif sector.type == Sector.STARBASE:
	//                 print "\07*** STAR BASE DESTROYED ***  .......CONGRATULATIONS"
	//                 quadrant.setsector(Sector(X, Y))
	//             else:
	//                 print "HIT SOMETHING UNKNOWN"
	//                 quadrant.setsector(Sector(X, Y))

	//         self.checkForDamage(gamestate)
	//         if self.energy < 0:
	//             print "OUT OF ENERGY"
	//             gamestate.DESTROYED=True

	void Ship::shieldControl(game_session *gamestate) {
	    if (damage("computer") < 0) {
		cout << "SHIELD CONTROL IS NON-OPERATIONAL" << endl;
	    } else {
		int orig_energy = energy_;
		int orig_shields = shields_;
		do {
		    cout << "ENERGY AVAILABLE =" << (energy_ + shields_) << "   NUMBER OF UNITS TO SHIELDS:?";
		    int X = 0;
		    try {
			cin >> X;
		    } catch (exception *e) {
			X = -1;
		    }
		    if (X <= 0) {
			energy_ = orig_energy;
			shields_ = orig_shields;
			return;
		    }
		    energy_ += shields_ - X;
		    shields_ = X;
		} while (energy_ < 0 || shields_ < 0);
	    }
	}
    
	//     def computerControl(self, gamestate):
	//         if self.damage("computerctl") < 0:
	//             print "COMPUTER DISABLED"
	//             return
	//         while True:
	//             print "COMPUTER ACTIVE AND AWAITING COMMAND",
	//             try:
	//                 A = input(": ")
	//             except:
	//                 A = -1
	//             if A == 0:
	//                 self.printGalacticRecord(gamestate)
	//                 return
	//             if A == 1:
	//                 self.printStatusReport(gamestate)
	//                 return
	//             if A == 2:
	//                 self.photonTorpedoData(gamestate)
	//                 return
	//             print "\nFUNCTIONS AVAILABLE FROM COMPUTER\n"
	//       	print "   0 = CUMULATIVE GALATIC RECORD"
	//       	print "   1 = STATUS REPORT"
	//       	print "   2 = PHOTON TORPEDO DATA\n"

	//     def printGalacticRecord(self, gamestate):
	//         print "COMPUTER RECORD OF GALAXY FOR QUADRANT %d,%d" % (self.Q1,self.Q2)
	//         print "     0     1     2     3     4     5     6     7"
	//         print "   ----- ----- ----- ----- ----- ----- ----- -----"
	//         for I in range(8):
	//             print ("%d " % (I)),
	//             for J in range(8):
	//                 q = self.computer_galaxy_scan.quadrant(I,J)
	//                 print (" %d%d%d " % (q.num_klingons, q.num_starbases, q.num_stars)),
	//             print "\n   ----- ----- ----- ----- ----- ----- ----- -----"

	//     def printStatusReport(self, gamestate):
	//         print "\n   STATUS REPORT\n"
	//         print "NUMBER OF KLINGONS LEFT  = %d" % (Galaxy::galaxy.remaining_klingons)
	//         print "NUMBER OF STARDATES LEFT = %d" % (T0+MAX_STARDATES-gamestate.T)
	//         print "NUMBER OF STARBASES LEFT = %d" % (Galaxy::galaxy.remaining_starbases)
	//         self.printDamageControlReport(gamestate)

	//     def photonTorpedoData(self, gamestate):
	//         print 
	//         for klingon in Galaxy::galaxy.quadrant(self.Q1, self.Q2).klingons():
	//             if klingon.shields > 0:
	//                 C1=self.pos.row
	//                 A=self.pos.col
	//                 W1=klingon.pos.row
	//                 X=klingon.pos.row
	//                 printDistanceAndDirection(C1,A,W1,X)

	//         while True:
	//             A=""
	//             while A[0] != 'Y' and A[0] != 'N':
	//                 print "DO YOU WANT TO USE THE CALCULATOR",
	//                 try:
	//                     A = raw_input("? ")
	//                 except:
	//                     A = 'N'

	//             if A[0] == "N":
	//                 return

	//             //"calculator"
	//             print "YOU ARE AT QUADRANT ( %d,%d )  SECTOR ( %d,%d )" % (self.Q1,self.Q2,self.pos.row,self.pos.col)
	//             print "SHIP'S & TARGET'S COORDINATES ARE",
	//             try:
	//                 str = raw_input("? ")
	//                 (C1,A,W1,X) = str.split()
	//                 printDistanceAndDirection(C1,A,W1,X)
	//             except:
	//                 print "INPUT GARBLED"

	// // ===================================

	// def printDir(ship_r, ship_c, targ_r, targ_c):
	//     xdelta=targ_c-ship_c
	//     ydelta=ship_r-targ_r
	//     if xdelta>=0:
	//         if ydelta<0:
	//              if ABS(ydelta) < ABS(xdelta):
	//                  print "DIRECTION =%f" % (ship_r+(((ABS(xdelta)-ABS(ydelta))+ABS(xdelta))/ABS(xdelta)))
	//              else:
	//                  print "DIRECTION =%f" % (ship_r+(ABS(xdelta)/ABS(ydelta)))
	//              return (xdelta,ydelta)
	//         if xdelta<=0:
	//             if ydelta==0:
	//                 ship_r=5
	//                 if ABS(ydelta) <= ABS(xdelta):
	//                     print "DIRECTION =%f" % (ship_r+(ABS(ydelta)/ABS(xdelta)))
	//                 else:
	//                     print "DIRECTION =%f" % (ship_r+(((ABS(ydelta)-ABS(xdelta))+ABS(ydelta))/ABS(ydelta)))
	//                 return (xdelta,ydelta)
	//         ship_r=1
	//         if ABS(ydelta) > ABS(xdelta):
	//             print "DIRECTION =%f" % (ship_r+(((ABS(ydelta)-ABS(xdelta))+ABS(ydelta))/ABS(ydelta)))
	//             return (xdelta,ydelta)
	//         print "DIRECTION =%f" % (ship_r+(ABS(ydelta)/ABS(xdelta)))
	//         return (xdelta,ydelta)
	//     if ydelta>0:
	//         ship_r=3
	//         if ABS(ydelta) >= ABS(xdelta):
	//             print "DIRECTION =%f" % (ship_r+(ABS(xdelta)/ABS(ydelta)))
	//         else:
	//             print "DIRECTION =%f" % (ship_r+(((ABS(xdelta)-ABS(ydelta))+ABS(xdelta))/ABS(xdelta)))
	//         return (xdelta,ydelta)
	//     if xdelta != 0:
	//         ship_r=5
	//         if ABS(ydelta) <= ABS(xdelta):
	//             print "DIRECTION =%f" % (ship_r+(ABS(ydelta)/ABS(xdelta)))
	//         else:
	//             print "DIRECTION =%f" % (ship_r+(((ABS(ydelta)-ABS(xdelta))+ABS(ydelta))/ABS(ydelta)))
	//     else:
	//         ship_r=7
	//         if ABS(ydelta) < ABS(xdelta):
	//             print "DIRECTION =%f" % (ship_r+(((ABS(xdelta)-ABS(ydelta))+ABS(xdelta))/ABS(xdelta)))
	//         else:
	//             print "DIRECTION =%f" % (ship_r+(ABS(xdelta)/ABS(ydelta)))

	// def printDistanceAndDirection(ship_r, ship_c, targ_r, targ_c):
	//     [xdelta,ydelta] = printDir(ship_r, ship_c, targ_r, targ_c)
	//     print "DISTANCE =%f" % (sqrt(pow(xdelta,2)+pow(ydelta,2)))

	// // ===================================

	// def setCourse(gamestate, ship):
	//     ship.setCourse(gamestate)

	// def printShortRangeSensorScan(gamestate, ship):
	//     ship.srscan(gamestate)

	// def printLongRangeSensorScan(gamestate, ship):
	//     ship.lrscan(gamestate)

	// def firePhasers(gamestate, ship):
	//     ship.firePhasers(gamestate)

	// def firePhotons(gamestate, ship):
	//     ship.firePhotons(gamestate)

	// def shieldControl(gamestate, ship):
	//     ship.shieldControl(gamestate)

	// def printDamageControlReport(gamestate, ship):
	//     ship.printDamageControlReport(gamestate)

	// def computerControl(gamestate, ship):
	//     ship.computerControl(gamestate)
    
    void game_session::showInstructions() {

	cout << "\n     INSTRUCTIONS:\n\n" << endl;
	cout << "<*> = ENTERPRISE" << endl;
	cout << "+++ = KLINGON" << endl;
	cout << ">!< = STARBASE" << endl;
	cout << " *  = STAR" << endl;
	cout << "\n\n\nCOMMAND 0 = WARP ENGINE CONTROL" << endl;
	cout << "  \"COURSE\" IS IN A CIRCULAR NUMERICAL          4  3  2" << endl;
	cout << "  VECTOR ARRANGEMENT AS SHOWN.                  \\ ^ /" << endl;
	cout << "  INTERGER AND REAL VALUES MAY BE                \\^/" << endl;
	cout << "  USED.  THEREFORE COURSE 1.5 IS              5 ------ 1" << endl;
	cout << "  HALF WAY BETWEEN 1 AND 2.                      /^\\" << endl;
	cout << "                                                / ^ \\" << endl;
	cout << "  A VECTOR OF 9 IS UNDEFINED, BUT              6  7  8" << endl;
	cout << "  VALUES MAY APPROACH 9." << endl;
	cout << "                                               COURSE" << endl;
	cout << "  ONE \"WARP FACTOR\" IS THE SIZE OF" << endl;
	cout << "  ONE QUADRANT.  THEREFORE TO GET" << endl;
	cout << "  FROM QUADRANT 6,5 TO 5,5 YOU WOULD" << endl;
	cout << "  USE COURSE 3, WARP FACTOR 1" << endl;
	cout << "\n\nCOMMAND 1 = SHORT RANGE SENSOR SCAN" << endl;
	cout << "  PRINTS THE QUADRANT YOU ARE CURRENTLY IN, INCLUDING" << endl;
	cout << "  STARS, KLINGONS, STARBASES, AND THE ENTERPRISE; ALONG" << endl;
	cout << "  WITH OTHER PERTINATE INFORMATION." << endl;
	cout << "\n\nCOMMAND 2 = LONG RANGE SENSOR SCAN" << endl;
	cout << "  SHOWS CONDITIONS IN SPACE FOR ONE QUADRANT ON EACH SIDE" << endl;
	cout << "  OF THE ENTERPRISE IN THE MIDDLE OF THE SCAN.  THE SCAN" << endl;
	cout << "  IS CODED IN THE FORM XXX, WHERE THE UNITS DIGIT IS THE" << endl;
	cout << "  NUMBER OF STARS, THE TENS DIGIT IS THE NUMBER OF STAR-" << endl;
	cout << "  BASES, THE HUNDREDS DIGIT IS THE NUMBER OF KLINGONS." << endl;
	cout << "\n\nCOMMAND 3 = PHASER CONTROL" << endl;
	cout << "  ALLOWS YOU TO DESTROY THE KLINGONS BY HITTING HIM WITH" << endl;
	cout << "  SUITABLY LARGE NUMBERS OF ENERGY UNITS TO DEPLETE HIS " << endl;
	cout << "  SHIELD POWER.  KEEP IN MIND THAT WHEN YOU SHOOT AT" << endl;
	cout << "  HIM, HE GONNA DO IT TO YOU TOO." << endl;
	cout << "\n\nCOMMAND 4 = PHOTON TORPEDO CONTROL" << endl;
	cout << "  COURSE IS THE SAME AS USED IN WARP ENGINE CONTROL" << endl;
	cout << "  IF YOU HIT THE KLINGON, HE IS DESTROYED AND CANNOT FIRE" << endl;
	cout << "  BACK AT YOU.  IF YOU MISS, HE WILL SHOOT HIS PHASERS AT" << endl;
	cout << "  YOU." << endl;
	cout << "   NOTE: THE LIBRARY COMPUTER (COMMAND 7) HAS AN OPTION" << endl;
	cout << "   TO COMPUTE TORPEDO TRAJECTORY FOR YOU (OPTION 2)." << endl;
	cout << "\n\nCOMMAND 5 = SHIELD CONTROL" << endl;
	cout << "  DEFINES NUMBER OF ENERGY UNITS TO BE ASSIGNED TO SHIELDS" << endl;
	cout << "  ENERGY IS TAKEN FROM TOTAL SHIP'S ENERGY." << endl;
	cout << "\n\nCOMMAND 6 = DAMAGE CONTROL REPORT" << endl;
	cout << "  GIVES STATE OF REPAIRS OF ALL DEVICES.  A STATE OF REPAIR" << endl;
	cout << "  LESS THAN ZERO SHOWS THAT THAT DEVICE IS TEMPORARALY" << endl;
	cout << "  DAMAGED." << endl;
	cout << "\n\nCOMMAND 7 = LIBRARY COMPUTER" << endl;
	cout << "  THE LIBRARY COMPUTER CONTAINS THREE OPTIONS:" << endl;
	cout << "    OPTION 0 = CUMULATIVE GALACTIC RECORD" << endl;
	cout << "     SHOWS COMPUTER MEMORY OF THE RESULTS OF ALL PREVIOUS" << endl;
	cout << "     LONG RANGE SENSOR SCANS" << endl;
	cout << "    OPTION 1 = STATUS REPORT" << endl;
	cout << "     SHOWS NUMBER OF KLINGONS, STARDATESC AND STARBASES" << endl;
	cout << "     LEFT." << endl;
	cout << "    OPTION 2 = PHOTON TORPEDO DATA" << endl;
	cout << "     GIVES TRAJECTORY AND DISTANCE BETWEEN THE ENTERPRISE" << endl;
	cout << "     AND ALL KLINGONS IN YOUR QUADRANT\n\n\n\n\n" << endl;
    }

    bool game_session::mainloop() {
	for (int i=0; i < 10; i++) {
	    cout << endl;
	}
	cout << "                          STAR TREK " << endl;
	cout << "\n\n\nDO YOU WANT INSTRUCTIONS (THEY'RE LONG!)? ";
	string answer = "";
	try {
	    cin >> answer;
	} catch (exception& e) {
	    answer = "";
	}
	if (answer == "YES") {
	    game_session::showInstructions();
	}

	//*****  PROGRAM STARTS HERE *****
	cout << "\n\n\n\n\n\n\n\n\n\n\n\n";

	//"T0: stardate mission began"
	//"T: current stardate"
	T_ = 2000 + (20 * randrange(100));
	T0_ = T_;

	//"T7: the time at which the mission began"
	start_time_ = chrono::system_clock::to_time_t(chrono::system_clock::now());

	Ship enterprise = Ship();

	cout << "YOU MUST DESTROY "
	     << Galaxy::remaining_klingons() << " KLINGONS IN "
	     << MAX_STARDATES << " STARDATES WITH "
	     << Galaxy::remaining_starbases() << " STARBASES" << endl;

	RESTART=false;
	while (!GAMEOVER && !RESTART) {
	    // XXX - debug code
	    // for (int row = 0; row < 8; row++) {
	    // 	for (int col = 0; col < 8; col++) {
	    // 	    if (Galaxy::galaxy.quadrant(enterprise.q_row(),enterprise.q_col()).sector(row,col).type == Sector.KLINGON) {
	    // 		print "SECTOR(%d,%d)=KLINGON" % (row,col);
	    // 	    }
	    // 	}
	    // }
        
	    cout << "ENTERING QUADRANT " << enterprise.q_row() << "," << enterprise.q_col() << endl;
	    Quadrant& q = Galaxy::quadrant(enterprise.q_row(),enterprise.q_col());
	    DGAME(1, "enterprise at secpos %d, %d\n", (int)enterprise.s_row(), (int)enterprise.s_col());
	    auto s = Starship(enterprise.s_row(), enterprise.s_col());
	    enterprise.pos(s.row(), s.col());
	    q.setsector(s, true);
	    auto qs = q.sector(s.row(), s.col());
	    DGAME(2, "q's sector %d,%d is %s (%d)", s.row(), s.col(), qs.glyph().c_str(), qs.id());
	    if (q.num_klingons() > 0 && enterprise.shields()<=200) {
		cout << "COMBAT AREA      CONDITION RED" << endl;
		cout << "   SHIELDS DANGEROUSLY LOW" << endl;
	    }
	    enterprise.srscan(this);

	    DESTROYED=false;
	    SAMEQUADRANT=true;
	    while (SAMEQUADRANT && !RESTART && !GAMEOVER) {
		if (Galaxy::remaining_klingons()<=0) {
		    cout << endl << "THE LAST KLINGON BATTLE CRUISER IN THE GALAXY HAS BEEN DESTROYED" << endl;
		    cout << "THE FEDERATION HAS BEEN SAVED !!!" << endl << endl;
		    cout << "YOUR EFFICIENCY RATING =" << ((Galaxy::total_klingons()/(T_-T0_))*1000) << endl;
		    time_t T1 = chrono::system_clock::to_time_t(chrono::system_clock::now());
		    cout << "YOUR ACTUAL TIME OF MISSION = " << (trunc((T1-start_time_)/60.0)) << " MINUTES" << endl;
		    RESTART=true;
		} else if (DESTROYED) {
		    cout << endl <<  "THE ENTERPRISE HAS BEEN DESTROYED. THE FEDERATION WILL BE CONQUERED" << endl;
		    GAMEOVER=true;
		} else if (T_ > (T0_+MAX_STARDATES)) {
		    cout << endl << "IT IS STARDATE " << T_ << endl;
		    cout << "MISSION TIME LIMIT EXPIRED.  YOU HAVE FAILED." << endl;
		    RESTART=true;
		} else {
		    cout << "COMMAND:? ";
		    string cmd { "" };
		    try {
			cin >> cmd;
		    } catch (exception& e) {
			cmd = "";
		    }
		    if (cmd[0] == '-') {
			return true;
		    }
		    int n = strtol(cmd.c_str(), NULL, 10);
		    switch (n) {
		    case 0: enterprise.setCourse(this); break;
		    case 1: enterprise.srscan(this); break;
		    case 2: enterprise.lrscan(this); break;
		    case 3: enterprise.firePhasers(this); break;
		    case 4: enterprise.firePhotons(this); break;
		    case 5: enterprise.shieldControl(this); break;
		    case 6: enterprise.printDamageControlReport(this); break;
		    case 7: enterprise.computerControl(this); break;
		    default:
			cout << endl;
			cout << "   0 = SET COURSE" << endl;
			cout << "   1 = SHORT RANGE SENSOR SCAN" << endl;
			cout << "   2 = LONG RANGE SENSOR SCAN" << endl;
			cout << "   3 = FIRE PHASERS" << endl;
			cout << "   4 = FIRE PHOTON TORPEDOES" << endl;
			cout << "   5 = SHIELD CONTROL" << endl;
			cout << "   6 = DAMAGE CONTROL REPORT" << endl;
			cout << "   7 = CALL ON LIBRARY COMPUTER" << endl;
			cout << "  -1 = QUIT" << endl;
			cout << endl;
		    }
		}
	    }
	}
	
	if (Galaxy::remaining_klingons()>0) {
	    cout << "THERE ARE STILL " << Galaxy::remaining_klingons() << " KLINGON BATTLE CRUISERS" << endl;
	}

	return true;
    }

    int my_main(int argc, char **argv) {
	DVERB(1, "entry");
	game_session game = game_session();
	while (false == game.mainloop()) {
	    game = game_session();
	}
	return 0;
    }

} // namespace std

int main(int argc, char **argv) {
    try {
	//	DBGENABLE(verbose);
	DBGENABLE(game);
	dbg::g_level = 1;
	return std::my_main(argc, argv);
    } catch (std::exception& e) {
	std::cout << "Died from an exception" << e.what() << std::endl;
    }
    return 1;
}

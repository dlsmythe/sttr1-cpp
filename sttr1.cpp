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
#include <cctype>
#include <getopt.h>
#include <cstring>

namespace dbg {

    enum class flag {
	verbose,
	sector,
	    quadrant,
	    galaxy,
	    ship,
	    init,
	    game,
	    
	    numflags
	    };
    using dbgflags = typename std::bitset<static_cast<int>(dbg::flag::numflags)>;
    dbgflags g_flags {};
    int g_level = 1;

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
#define DSHIP(LVL,FMT,...) dbg::print(dbg::flag::ship, (LVL), __FILE__, __FUNCTION__, __LINE__, FMT, ## __VA_ARGS__)
#define DINIT(LVL,FMT,...) dbg::print(dbg::flag::init, (LVL), __FILE__, __FUNCTION__, __LINE__, FMT, ## __VA_ARGS__)
#define DGAME(LVL,FMT,...) dbg::print(dbg::flag::game, (LVL), __FILE__, __FUNCTION__, __LINE__, FMT, ## __VA_ARGS__)
#define DEXEC(LVL,FLG,...) do { if (dbg_predicate((FLG),(LVL))) { __VA_ARGS__ }}while (false)
}

namespace std {

    const float PI = 3.1415926535897932;

    const int KLINGON_MAX_SHIELDS=200;

    // MAX_STARDATES: maximum number of stardates the mission contains
    const int MAX_STARDATES=30;


    int randrange(int n) {
	return trunc(n * ((rand()-1) / (float)RAND_MAX));
    }

    string choice(unordered_map<string,string> m) {
	int i = randrange(m.size());
	for (auto x : m) {
	    if (i-- == 0) {
		return x.first;
	    }
	}
	return "error";
    }

    
    float course_to_radians(float course) {
	return ((course - 1) / 8.0) * 2 * PI;
    }

    
    float radians_to_course(float radians) {
	return 1.0 + 8 * (radians / (2*PI));
    }

    // ===================================
    // Determining direction:
    // The direction is derivable from the angle of the right-triangle formed by
    // the X and Y axes and the vector to the target.  The only question is the signs
    // of the components.  So, determine which quadrant the ship and target are in,
    // determine the direction based on quadrant 0:
    //   1 | 0
    //  -------
    //   2 | 3
    // then convert the result based on the actual quadrant.
    // NB: (0,0) is the upper left sector, (n,n) is lower right (for positive n).
    // (SOH CAH TOA)
    // distance = sqrt( (xdelta)^2 + (ydelta)^2 )
    // Q0: angle is asin(ydelta/distance) [call it Q0a]
    // Q1: angle is PI - Q0a
    // Q2: angle is PI + Q0a
    // Q3: angle is 2*PI - Q0a

    //const float EPSILON = .0001;
    pair<int,int> distance_and_direction(int ship_r, int ship_c, int targ_r, int targ_c) {
	float xdelta=targ_c-ship_c;
	float ydelta=targ_r-ship_r;
	float dist = sqrt(pow(xdelta,2)+pow(ydelta,2));
	float angle, Q0a = asin(abs(ydelta)/dist); // ship and target are in different sectors, so dist will never be zero.
	if (xdelta > 0) {
	    if (ydelta > 0) {          // Quadrant 3
		angle = 2*PI - Q0a;
	    } else {                   // Quadrant 0
		angle = Q0a;
	    }
	} else {
	    if (ydelta > 0) {          // Quadrant 2
		angle = PI + Q0a;
	    } else {                   // Quadrant 1
		angle = PI - Q0a;
	    }
	}
    
	return pair<int,int> {dist, radians_to_course(angle)};
    }
    
    // row,col in [0,8)
    // course in [1,9)
    // quadrant position wraps around.
    // returns (qrow,qcol,srow,scol)
    tuple<int,int,int,int> dest_in_dir_with_dist(int start_row, int start_col, float course, float warp_factor) {
	float angle = course_to_radians(course);
	float delta_sec_rows = warp_factor * 8 * -1 * sin(angle);
	float delta_sec_cols = warp_factor * 8 * cos(angle);
	float dest_quadrant_row = fmod(start_row + delta_sec_rows / 8, 8);
	float dest_quadrant_col = fmod(start_col + delta_sec_cols / 8, 8);
	int dest_sec_row = static_cast<int>(floor(fmod(delta_sec_rows, 8.0)));
	int dest_sec_col = static_cast<int>(floor(fmod(delta_sec_cols, 8.0)));
	return make_tuple(dest_quadrant_row, dest_quadrant_col, dest_sec_row, dest_sec_col);
    }

    //C[]: course direct array. determines sign of movement increment in a given direction.
    const float C[2][8] {
	{  0, -1, -1, -1,  0,  1,  1,  1 },	// row - direction of vertical component towards target
	{  1,  1,  0, -1, -1, -1,  0,  1 }	// col - direction of horizontal component towards target
    };
    
    pair<int,int> next_pos_toward_dest(int start_row, int start_col, int dest_row, int dest_col) {
	int shortest_dist_course;
	float shortest_dist = 10000000000;
	for (int i = 0; i < 8; i++) {
	    float dist, dir;
	    tie(dist, dir) = distance_and_direction(start_row + C[0][i], start_col + C[1][i],
						    dest_row, dest_col);
	    if (dist < shortest_dist) {
		shortest_dist = dist;
		shortest_dist_course = i;
	    }
	}
	return pair<int,int>(start_row + C[0][shortest_dist_course],
			     start_col + C[1][shortest_dist_course]);
    }

    pair<int, int> next_pos_on_course(int start_row, int start_col, float course) {
	if (start_row < 0 || start_row >= 8 || start_col < 0 || start_col >= 8) {
	    throw invalid_argument("start position invalid");
	}
	if (course < 1 || course > 9) {
	    throw invalid_argument("course invalid");
	}
	int r_course = static_cast<int>(round(course));
	if (r_course == 9) {
	    r_course = 0;
	}
	int nrow = start_row + C[0][r_course - 1];
	int ncol = start_col + C[1][r_course - 1];
	if (nrow < 0 || nrow >= 8 || ncol < 0 || ncol >= 8) {
	    throw out_of_range("moved to new quadrant");
	}
	return pair<int,int>(nrow, ncol);
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
	Sector(int row, int col, Sector::type type = Sector::type::SPACE, string glyph = "   "):
	    pos_(SectorPos(row,col)), type_(type), id_(ID++), glyph_(glyph) {
	    DSECTOR(2, "new space %d at sector %d,%d", id(), row, col);
	}
	Sector(const Sector& rhs): pos_(rhs.pos_), type_(rhs.type_), id_(ID++), glyph_(rhs.glyph_) {
	    DSECTOR(2, "copy-constructing sector with id %d to new one with id %d", rhs.id_, id_);
	}
	Sector(const Sector&& rhs): pos_(rhs.pos_), type_(rhs.type_), id_(ID++), glyph_(rhs.glyph_) {
	    DSECTOR(2, "move-constructing sector with id %d to new one with id %d", rhs.id_, id_);
	}
	Sector& operator=(const Sector& p) {
	    id_ = ID++;
	    pos_ = p.pos_;
	    type_ = p.type_;
	    glyph_ = p.glyph_;
	    DSECTOR(2, "copy-assigning sector with id %d to new one with id %d", p.id_, id_);
	    return *this;
	}
	Sector& operator=(const Sector&& p) {
	    id_ = ID++;
	    pos_ = p.pos_;
	    type_ = p.type_;
	    glyph_ = p.glyph_;
	    DSECTOR(2, "move-assigning sector with id %d to new one with id %d", p.id_, id_);
	    return *this;
	}
	virtual ~Sector() {
	    DSECTOR(2, "destructing sector with id %d", id_);
	}
	virtual string glyph() { return glyph_; }
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
	string glyph_ = "   ";
    };
    int Sector::ID = 0;
    
    class Klingon : public Sector {
    public:
	Klingon(int row, int col): Sector(row,col,Sector::type::KLINGON, "+++") {
	    DSECTOR(2, "new Klingon %d at sector %d,%d", id(), row, col);
	}
	Klingon(Klingon& rhs): Sector(rhs.row(),rhs.col(),Sector::type::KLINGON, "+++") {
	    shields_ = rhs.shields_;
	    DSECTOR(2, "copied Klingon %d at sector %d,%d", id(), row(), col());
	}
	virtual int shields() override { return shields_; }
	virtual void shields(int val) override { shields_ = val; }
    private:
	int shields_ = 0;
    };
	
    class Starship : public Sector {
    public:
	Starship(int row, int col): Sector(row,col,Sector::type::STARSHIP, "<*>") {
	    DSECTOR(2, "new Starship %d at sector %d,%d", id(), row, col);
	}
    };
	
    class Starbase : public Sector {
    public:
	Starbase(int row, int col): Sector(row,col,Sector::type::STARBASE, ">!<") {
	    DSECTOR(2, "new Starbase %d at sector %d,%d", id(), row, col);
	}
    };
	
    class Star : public Sector {
    public:
	Star(int row, int col): Sector(row,col,Sector::type::STAR, " * ") {
	    DSECTOR(2, "new Star %d at sector %d,%d", id(), row, col);
	}
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
	unique_ptr<Sector> sectors_[8][8];
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
	    DQUAD(1, "q(%d,%d)[%d][%d]: id %d type: %d [%s]", g_row_, g_col_, row, col,
		  sectors_[row][col]->id(), sectors_[row][col]->type(), sectors_[row][col]->glyph().c_str());
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

	static Galaxy& the_galaxy() {
	    if (nullptr == singleton) {
		singleton = unique_ptr<Galaxy>(new Galaxy());
	    }
	    return *singleton;
	}
	
	static pair<int,int> randomemptypos(int qrow, int qcol) {
	    return the_galaxy().galaxy_[qrow][qcol].randomemptypos();
	}
    
	static Quadrant& quadrant(int row, int col) {
	    return the_galaxy().galaxy_[row][col];
	}

	static int total_klingons() {
	    return the_galaxy().total_klingons_;
	}

	static int remaining_klingons() {
	    return the_galaxy().remaining_klingons_;
	}

	static void decr_remaining_klingons() {
	    --the_galaxy().remaining_klingons_;
	}

	static int remaining_starbases() {
	    return the_galaxy().remaining_starbases_;
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
	void increment_stardate() { ++T_; }
	int initial_stardate() { return T0_; }
	void gameover(bool state) { GAMEOVER_ = state; }
	bool gameover() { return GAMEOVER_; }
	void destroyed(bool state) { DESTROYED_ = state; }
	bool destroyed() { return DESTROYED_; }
	bool SAMEQUADRANT() { return SAMEQUADRANT_; }
	void SAMEQUADRANT(bool state) { SAMEQUADRANT_ = state; }
	void RESTART_REQUESTED(bool state) { RESTART_REQUESTED_ = state; }
	bool RESTART_REQUESTED() { return RESTART_REQUESTED_; }

    private:
	bool GAMEOVER_ = false;
	bool DESTROYED_ = false;
	bool SAMEQUADRANT_ = false;
	bool RESTART_REQUESTED_ = false;
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

	struct cgs_s { int ks; int sbs; int stars; };
	
	Ship(): DOCKED(false), photons_(MAXPHOTONS), energy_(MAXENERGY), shields_(0), condition(Ship::cond::GREEN) {

	    clear_damage();

	    //X and Y coordinates of the current quadrant, 0..7 each
	    q_row_ = randrange(8);
	    q_col_ = randrange(8);

	    //S1 and S2, X and Y coordinates of the ship in the current quadrant, 1..8 each
	    int r, c;
	    tie(r, c) = Galaxy::randomemptypos(q_row_,q_col_);
	    pos_ = SectorPos(r,c);
	}

	int q_row() { return q_row_; }
	int q_col() { return q_col_; }
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
	map<string, int> curdamage_;	//"ship.damage(): damage array.  Values < 0 indicate damage"
	int photons_;
	int energy_;
	int shields_;
	SectorPos pos_ = SectorPos(0,0);
	int q_row_, q_col_; // ship's quadrant (r,c)
	enum Ship::cond condition;
	struct cgs_s computer_galaxy_scan_[8][8] {};

	void clear_damage(void);
	int damage(string sysname) { return curdamage_[sysname]; }
	string devicename(string sysname) { return system_names[sysname]; }
	void checkForDamage(game_session *gamestate);
	void move(game_session *gamestate, float course, float warp_factor);
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
	DGAME(1, "checking whether docked...");
	DOCKED=false;
	for (int I = s_row()-1; I <= s_row()+1 && !DOCKED; I++) {
	    for (int J = s_col()-1; J <= s_col()+1 && !DOCKED; J++) {
		if (I>=0 && I<=7 && J>=0 && J<=7) {
		    if (quadrant.sector(s_row(), s_col()).type() == Sector::type::STARBASE) {
			energy_ = MAXENERGY;
			photons_ = MAXPHOTONS;
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

	DGAME(1, "printing quadrant sectors...");
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
	cout << "        PHOTON TORPEDOES        " << photons_ << endl;
	for (int col = 0; col < 8; col++) {
	    cout << setw(3) << quadrant.sector(7,col).glyph();
	}
	cout << "        SHIELDS         	" << shields_ << endl;
	cout << "---------------------------------------" << endl;
	DGAME(1, "QUADRANT PRINT COMPLETE");
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
			    computer_galaxy_scan_[I][J].ks = quadrant.num_klingons();
			    computer_galaxy_scan_[I][J].sbs = quadrant.num_starbases();
			    computer_galaxy_scan_[I][J].stars = quadrant.num_stars();
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
	    for (auto klingon : Galaxy::quadrant(q_row_,q_col_).klingons()) {
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
    
    void Ship::move(game_session *gamestate, float course, float warp_factor) {
	//"erase our current position?"
	DSHIP(1, "Removing starship at [%d,%d]->sector(%d,%d)", q_row(),q_col(),s_row(), s_col());
	Galaxy::quadrant(q_row(),q_col()).setsector(Sector(s_row(), s_col()));

	int dest_qrow, dest_qcol, dest_srow, dest_scol;
	tie(dest_qrow, dest_qcol, dest_srow, dest_scol) = dest_in_dir_with_dist(s_row(), s_col(), course, warp_factor);

	//"quadrant is 8 units across.  Warp factor is a fraction of that."
	int N=trunc(warp_factor*8);

	//"move in N steps, checking at each step for collisions"
	for (int I = 0; I < N; I++) {
	    DSHIP(1, "(%d, %d)", s_row(), s_col());
	    int nrow, ncol;
	    try {
		// "calculate the incremental delta movement"
		tie(nrow, ncol) = next_pos_on_course(s_row(), s_col(), course);
	    } catch (invalid_argument& e) {
		cout << e.what() << endl;
		exit(1);
	    } catch (out_of_range& e) {
	        //"flew out of the quadrant."
	        DSHIP(1, "flew out of the quadrant. dest: Q(%d,%d) S(%d,%d)",
		      dest_qrow, dest_qcol, dest_srow, dest_scol);

		q_row_ = dest_qrow;
		q_col_ = dest_qcol;
		pos_ = SectorPos(dest_srow, dest_scol);
	        gamestate->increment_stardate(); // "time passes"
	        energy_ -= N+5;
	        gamestate->SAMEQUADRANT(false);
	        return;
	    } catch (exception& e) {
		cout << e.what() << endl;
		exit(1);
	    } catch (...) {
		cout << "bad..." << endl;
		exit(1);
	    }

	    if (Galaxy::quadrant(q_row(),q_col()).sector(nrow, ncol).type() != Sector::type::SPACE) {
	        cout << " WARP ENGINES SHUTDOWN AT SECTOR " << s_row() << "," << s_col() << " DUE TO BAD NAVIGATION" << endl;
	        break;
	    }
	    pos(nrow, ncol);
	}

	// NOT a new quadrant...
        
	//"spend the energy it took to move here"
	energy_ -= (N - shields_);
	if (warp_factor>=1) {
	    gamestate->increment_stardate(); // "time passes"
	}

	//"no collisions -- set our new position"
	Galaxy::quadrant(q_row(),q_col()).setsector(Starship(s_row(),s_col())); // XXX - possibly redundant -- see mainloop

	srscan(gamestate);
    }

    void Ship::setCourse(game_session *gamestate) {
	float course=0, warp_factor=-1;
	while (course<1 or course>=9) {
	    cout << "COURSE (1-9):";
	    try {
		string str;
		getline(cin, str);
		stringstream conv(str);
		conv >> course;
	    } catch (exception *e) {
	        course = 0;
	    }
	    if (course==0) {
	        return;
	    }
	}

	while (warp_factor<0 or warp_factor>8) {
	    cout << "WARP FACTOR (0-8):";
	    try {
		string str;
		getline(cin, str);
		stringstream conv(str);
		conv >> warp_factor;
	    } catch (exception *e) {
		warp_factor = 0;
	    }
	    if (warp_factor>=0 && warp_factor<=8) {
		if (curdamage_["warpdrive"]<0 && warp_factor > .2) {
		    cout << "WARP ENGINES ARE DAMAGED, MAXIMUM SPEED = WARP .2" << endl;
		    warp_factor=-1;
		}
	    }
	    if (warp_factor < 0) {
		return;
	    }
	}

	if (Galaxy::quadrant(q_row(),q_col()).num_klingons() > 0) {
	    checkForDamage(gamestate);
	}
	if (gamestate->destroyed()) {
	    return;
	}

	if (energy_ <= 0) {
	    if (shields_ < 1) {
		cout << "THE ENTERPRISE IS DEAD IN SPACE. IF YOU SURVIVE ALL IMPENDING" << endl;
		cout << "ATTACK YOU WILL BE DEMOTED TO THE RANK OF PRIVATE" << endl;
		//"this is a death loop -- wait until either Enterprise is destroyed"
		//"or all Klingons in this quadrant self-destruct"
		while (!gamestate->destroyed()  &&  Galaxy::quadrant(q_row(),q_col()).num_klingons() > 0) {
		    checkForDamage(gamestate);
		}
		if (gamestate->destroyed()) {
		    cout << "THERE ARE STILL " << Galaxy::remaining_klingons() << " KLINGON BATTLE CRUISERS" << endl;
		    gamestate->RESTART_REQUESTED(true);
		    return;
		}
	    }
	    cout << "YOU HAVE " << energy_ << " UNITS OF ENERGY" << endl;
	    cout << "SUGGEST YOU GET SOME FROM YOUR SHIELDS WHICH HAVE " << shields_ << " UNITS LEFT" << endl;
	    return;
	}

	// always repair a little damage
	for (auto s : system_names) {
	    if (curdamage_[s.first] < 0) {
		curdamage_[s.first]++;  // XXX -- maybe be a little more generous?
	    }
	}

	// randomly cause or repair some damage
	if (randrange(10) <= 2) { // 30% chance
	    auto sname = choice(system_names);
	    cout << endl << "DAMAGE CONTROL REPORT:" << endl;
	    cout << devicename(sname);
	    if (randrange(2) == 0) {
		curdamage_[sname] -= randrange(5)+1;
		cout << " DAMAGED" << endl;
	    } else {
		curdamage_[sname] += randrange(5)+1;
		cout << " STATE OF REPAIR IMPROVED" << endl;
	    }
	}
		
	move(gamestate, course, warp_factor);
    }

    void Ship::firePhasers(game_session *gamestate) {
	auto& quadrant = Galaxy::quadrant(q_row(),q_col());
	if (quadrant.num_klingons() <= 0) {
	    cout << "SHORT RANGE SENSORS REPORT NO KLINGONS IN THIS QUANDRANT" << endl;
	    return;
	}
	if (curdamage_["phasers"] < 0) {
	    cout << "PHASER CONTROL IS DISABLED" << endl;
	    return;
	}
	if (curdamage_["computer"] < 0) {
	    cout << " COMPUTER FAILURE HAMPERS ACCURACY" << endl;
	}
	int X = 0;
	while (energy_ > X && X == 0) {
	    cout << "PHASERS LOCKED ON TARGET.  ENERGY AVAILABLE=" << energy_ << endl;
	    cout << "NUMBER OF UNITS TO FIRE:";
	    try {
		string str;
		getline(cin, str);
		stringstream conv(str);
		conv >> X;
	    } catch (exception *e) {
		cout << "INPUT GARBLED" << endl;
	        X = 0;
	    }
	    if (X < 0) {
	        return;
	    }
	    if (X > energy_) {
		cout << "INSUFFICIENT ENERGY TO SATISFY REQUEST." << endl;
	    }
	}
	energy_ -= X;
	checkForDamage(gamestate);
	if (gamestate->destroyed()) {
	    return;
	}
	if (curdamage_["computer"] < 0) {
	    X *= .2 + randrange(100)/100.;  //"computer failure result"	
	}
	for (auto klingon : quadrant.klingons()) {
	    if (klingon->shields() > 0) {
	        int H=(X/quadrant.num_klingons()/pos_.distance_to(klingon->pos()))*(2*(randrange(100)/100.));
	        klingon->shields(klingon->shields() - H);
	        cout << setw(4) << H << " UNIT HIT ON KLINGON AT SECTOR "
		     << klingon->row() << "," << klingon->col()
		     << "   (" << setw(3) << klingon->shields() << " LEFT)" << endl;
	        if (klingon->shields() < 0) {
	            cout << "KLINGON AT SECTOR " << klingon->row() << "," << klingon->col() << " DESTROYED ****" << endl;
	            quadrant.setsector(Sector(klingon->row(), klingon->col()));
	            Galaxy::decr_remaining_klingons();
	            if (Galaxy::remaining_klingons() <= 0) {
                        return;
		    }
		}
	    }
	}
	if (energy_ < 0) {
	    cout << "OUT OF ENERGY" << endl;
	    gamestate->destroyed(true);
	}
    }

    void Ship::firePhotons(game_session *gamestate) {
	if (curdamage_["photons"] < 0) {
	    cout << "PHOTON TUBES ARE NOT OPERATIONAL" << endl;
	    return;
	}
	if (photons_ <= 0) {
	    cout << "ALL PHOTON TORPEDOES EXPENDED" << endl;
	    return;
	}
	float course = 0;
	while (course < 1 || course >= 9) {
	    cout << "TORPEDO COURSE [1,9) (0 to cancel):";
	    try {
		string str;
		getline(cin, str);
		stringstream conv(str);
		conv >> course;
	    } catch (exception *e) {
		cout << "INPUT GARBLED" << endl;
		course = -1;
	    }
	    if (course == 0) {
		return;
	    }
	}

	--photons_;
	auto& quadrant = Galaxy::quadrant(q_row(),q_col());
	cout << "TORPEDO TRACK:" << endl;
	//"Loop until we hit something or torpedo leaves the quadrant"
	int row = s_row(), col = s_col();
	bool hit = false;
	while (hit == false) {
	    int nrow, ncol;
	    try {
		tie(nrow, ncol) = next_pos_on_course(row, col, course);
	    } catch (out_of_range& e) {
	        //"flew out of the quadrant."
	        cout << "TORPEDO MISSED" << endl;
	        break;
	    } catch (exception& e) {
		cout << e.what() << endl;
		exit(1);
	    }
	    row = nrow;
	    col = ncol;
	    cout << "               " << row << "," << col << endl;
	    if (quadrant.sector(row, col).type() != Sector::type::SPACE) {
	        hit = true;
	    }
	}
	if (hit) {
	    switch (quadrant.sector(row, col).type()) {
	    case Sector::type::STAR:
	        cout << "YOU CAN'T DESTROY STARS SILLY" << endl;
	        cout << "TORPEDO MISSED" << endl;
		break;
	    case Sector::type::KLINGON:
	        cout << "\07*** KLINGON DESTROYED ***" << endl;
	        quadrant.setsector(Sector(row, col));
	        Galaxy::decr_remaining_klingons();
	        if (Galaxy::remaining_klingons() <= 0) {
	            return;
		}
		break;
	    case Sector::type::STARBASE:
	        cout << "\07*** STAR BASE DESTROYED *** .......CONGRATULATIONS" << endl;
	        quadrant.setsector(Sector(row, col));
		break;
	    default:
	        cout << "HIT SOMETHING UNKNOWN" << endl;
	        quadrant.setsector(Sector(row, col));
		break;
	    }
	}
	checkForDamage(gamestate);
	if (energy_ < 0) {
	    cout << "OUT OF ENERGY" << endl;
	    gamestate->destroyed(true);
	}
    }

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
		    string str;
		    getline(cin, str);
		    stringstream conv(str);
		    conv >> X;
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
    
    void Ship::computerControl(game_session *gamestate) {
	if (curdamage_["computerctl"] < 0) {
	    cout << "COMPUTER DISABLED" << endl;
	    return;
	}

	while (true) {
	    cout << "COMPUTER ACTIVE AND AWAITING COMMAND: ";
	    int A = -1;
	    try {
		string str;
		getline(cin, str);
		stringstream conv(str);
		conv >> A;
	    } catch (exception *e) {
		A = -1;
	    }
	    switch (A) {
	    case 0:
		printGalacticRecord(gamestate);
		return;
	    case 1:
		printStatusReport(gamestate);
		return;
	    case 2:
		photonTorpedoData(gamestate);
		return;
	    default:
		cout << "\nFUNCTIONS AVAILABLE FROM COMPUTER" << endl << endl;
	      	cout << "   0 = CUMULATIVE GALATIC RECORD" << endl;
	      	cout << "   1 = STATUS REPORT" << endl;
	      	cout << "   2 = PHOTON TORPEDO DATA\n" << endl << endl;
		break;
	    }
	}
    }

    void Ship::printGalacticRecord(game_session *gamestate) {
	cout << "COMPUTER RECORD OF GALAXY FOR QUADRANT " << q_row() << "," << q_col() << endl;
	cout << "     0     1     2     3     4     5     6     7" << endl;
	cout << "   ----- ----- ----- ----- ----- ----- ----- -----" << endl;
	for (int I=0; I<8; I++) {
	    cout << " " << setw(1) << I << " ";
	    for (int J=0; J<8; J++) {
		cout << " "
		     << setw(1) << computer_galaxy_scan_[I][J].ks
		     << setw(1) << computer_galaxy_scan_[I][J].sbs
		     << setw(1) << computer_galaxy_scan_[I][J].stars
		     << "  ";
	    }
	    cout << endl << "   ----- ----- ----- ----- ----- ----- ----- -----" << endl;
	}
    }

    void Ship::printStatusReport(game_session *gamestate) {

	cout << endl << "   STATUS REPORT" << endl << endl;
	cout << "NUMBER OF KLINGONS LEFT  = " << Galaxy::remaining_klingons() << endl;
	cout << "NUMBER OF STARDATES LEFT = " << (gamestate->initial_stardate()+MAX_STARDATES-gamestate->stardate()) << endl;
	cout << "NUMBER OF STARBASES LEFT = " << Galaxy::remaining_starbases() << endl;
	printDamageControlReport(gamestate);
    }

    void printDistanceAndDirection(int ship_r, int ship_c, int targ_r, int targ_c) {
	float dist, dir;
	tie(dist, dir) = distance_and_direction(ship_r, ship_c, targ_r, targ_c);
	cout << "DIRECTION =" << dir << endl;
	cout << "DISTANCE  =" << dist << endl;
    }    
    
    // ===================================
    
    void Ship::photonTorpedoData(game_session *gamestate) {
	cout << endl;
	for (auto klingon : Galaxy::quadrant(q_row(), q_col()).klingons()) {
	    if (klingon->shields() > 0) {
		printDistanceAndDirection(s_row(), s_col(), klingon->row(), klingon->col());
	    }
	}

	while (true) {
	    string A = "";
	    do {
		// NB: why ask? Could we just default to showing data to all Klingons in this Quadrant?
	        cout << "DO YOU WANT TO USE THE CALCULATOR? ";
		cin.clear();
		try {
		    getline(cin, A);
		} catch (exception *e) {
		    A = "N";
		}
	    } while (A[0] != 'Y' and A[0] != 'N');

	    if (A[0] == 'N') {
	        return;
	    }

	    //"calculator"
	    cout << "YOU ARE AT QUADRANT ( " << q_row() << "," << q_col() << " )  SECTOR ( " << s_row() << "," << s_col() << " )" << endl;
	    cout << "SHIP'S & TARGET'S COORDINATES ARE? ";
	    int sr=0, sc=0, tr=0, tc=0;
	    try {
		string str;
	        getline(cin, str);
		stringstream conv(str);
		conv >> sr >> sc >> tr >> tc;
		printDistanceAndDirection(sr, sc, tr, tc);
	    } catch (exception *e) {
	        cout << "INPUT GARBLED" << endl;
	    }
	}
    }
    
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
	    getline(cin, answer);
	} catch (exception& e) {
	    answer = "";
	}
	if (answer == "YES") {
	    showInstructions();
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

	RESTART_REQUESTED(false);
	while (!gameover() && !RESTART_REQUESTED()) {
	    DEXEC(3, dbg::flag::game,
		  for (int row = 0; row < 8; row++) {
		      for (int col = 0; col < 8; col++) {
			  if (Galaxy::quadrant(enterprise.q_row(),enterprise.q_col()).sector(row,col).type() == Sector::type::KLINGON) {
			      cout << "SECTOR(" << row << "," << col << ")=KLINGON" << endl;
			  }
		      }
		  });
        
	    cout << "ENTERING QUADRANT " << enterprise.q_row() << "," << enterprise.q_col() << endl;
	    Quadrant& q = Galaxy::quadrant(enterprise.q_row(),enterprise.q_col());
	    DGAME(1, "enterprise at secpos %d, %d\n", (int)enterprise.s_row(), (int)enterprise.s_col());
	    auto s = Starship(enterprise.s_row(), enterprise.s_col());
	    enterprise.pos(s.row(), s.col());
	    q.setsector(s, true);
	    auto qs = q.sector(s.row(), s.col());
	    DGAME(2, "q's sector %d,%d is %s (%d) [%s]", s.row(), s.col(), qs.glyph().c_str(), qs.id(), qs.glyph().c_str());
	    if (q.num_klingons() > 0 && enterprise.shields()<=200) {
		cout << "COMBAT AREA      CONDITION RED" << endl;
		cout << "   SHIELDS DANGEROUSLY LOW" << endl;
	    }
	    enterprise.srscan(this);

	    destroyed(false);
	    SAMEQUADRANT(true);
	    while (SAMEQUADRANT() && !RESTART_REQUESTED() && !gameover()) {
		if (Galaxy::remaining_klingons() <= 0) {
		    cout << endl << "THE LAST KLINGON BATTLE CRUISER IN THE GALAXY HAS BEEN DESTROYED" << endl;
		    cout << "THE FEDERATION HAS BEEN SAVED !!!" << endl << endl;
		    cout << "YOUR EFFICIENCY RATING =" << ((Galaxy::total_klingons()/(T_-T0_))*1000) << endl;
		    time_t T1 = chrono::system_clock::to_time_t(chrono::system_clock::now());
		    cout << "YOUR ACTUAL TIME OF MISSION = " << (trunc((T1-start_time_)/60.0)) << " MINUTES" << endl;
		    RESTART_REQUESTED(true);
		} else if (destroyed()) {
		    cout << endl <<  "THE ENTERPRISE HAS BEEN DESTROYED. THE FEDERATION WILL BE CONQUERED" << endl;
		    gameover(true);
		} else if (T_ > (T0_+MAX_STARDATES)) {
		    cout << endl << "IT IS STARDATE " << T_ << endl;
		    cout << "MISSION TIME LIMIT EXPIRED.  YOU HAVE FAILED." << endl;
		    RESTART_REQUESTED(true);
		} else {
		    cout << "COMMAND:? ";
		    string cmd = "";
		    try {
			getline(cin, cmd);
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

	if (Galaxy::remaining_klingons() > 0) {
	    cout << "THERE ARE STILL " << Galaxy::remaining_klingons() << " KLINGON BATTLE CRUISERS" << endl;
	}

	return true;
    }

    int my_main(int argc, char **argv) {
	int c;
	while ((c = getopt(argc, argv, "D:")) != EOF) {
	    switch (c) {
	    case 'D':
		if (strstr("verbose", optarg)) {
		    DBGENABLE(verbose);
		} else if (strstr("game", optarg)) {
		    DBGENABLE(game);
		} else if (strstr("ship", optarg)) {
		    DBGENABLE(ship);
		}
		if (isdigit(optarg[0])) {
		    dbg::g_level = strtoul(optarg, NULL, 0);
		}
		break;
	    }
	}
	game_session game = game_session();
	while (false == game.mainloop()) {
	    game = game_session();
	}
	return 0;
    }

} // namespace std

int main(int argc, char **argv) {
    try {
	return std::my_main(argc, argv);
    } catch (std::exception& e) {
	std::cout << "Died from an exception" << e.what() << std::endl;
    }
    return 1;
}

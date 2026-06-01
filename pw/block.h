#define BK_EXTRACTED    0   /* this is the default */
#define BK_LIBRARY      1	/* float, unscaled */
#define BK_AVG          2	/* float, scaled */
#define XPOINTS         3	
#define BK_LIST		4   /*Block pdata comes another block, no stack*/

typedef struct {
    int npoints;
    int format;
    void *data;
} PointData;

void free_pdata(PointData *);
PointData *make_PointData(int, int, void *);
PointData *copy_PointData(PointData *);
double get_PointData(PointData *, int);

struct block_node {
    int             type;   /* type of node */
    PointData	    *pdata;
    PointData       *xpoints;
    int             color;  /* If some color other than block's default */
    struct stack_node *stack;   /* stack entry for easy reference */
    struct block_node *next;
};


struct block {
    int             state;
    int             nblocks;
    struct block_node *block;   /* list of points */
};

struct color_node {
    int             color;
    struct color_node *next;
};

struct stack_node {
    int             x, y;
    struct color_node *colors;
    struct stack_node *next;
};

struct PlotStruct {
    PointData *(*get)();
    PointData *(*scale)(int, PointData *, float);
    void (*draw)();
    void (*drawall)();
};
struct block_node *GetFirstBlock(int i);


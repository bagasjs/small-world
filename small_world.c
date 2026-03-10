#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "raylib.h"

#define NODE_REGION_CAPACITY 64
size_t links_cap = 10;

typedef enum {
    COLOR_NODE,
    COLOR_SELECTED_NODE,
    COLOR_BACKGROUND,
    _COUNT_COLORS,
} ColorPallete;

Color pallete[_COUNT_COLORS] = {
    [COLOR_NODE] = GREEN,
    [COLOR_SELECTED_NODE] = WHITE,
    [COLOR_BACKGROUND] = (Color){ 0x18, 0x18, 0x18, 0xFF },
};

typedef struct Node Node;
struct Node {
    size_t id;
    Node *next;
    Node **links;
    size_t links_count;
    size_t links_cap;
    ColorPallete color;

    int x;
    int y;
};

typedef struct NodeRegion NodeRegion;
struct NodeRegion {
    NodeRegion *next;
    size_t count;
    Node items[NODE_REGION_CAPACITY];
};

NodeRegion *first_region = NULL;
NodeRegion *last_region  = NULL;
Node *first_node = NULL;
Node *last_node  = NULL;
size_t total_nodes = 0;
size_t total_links = 0;

float avg_steps = 0;
size_t calc_total_nodes = 0;
size_t calc_total_links = 0;

void free_all_regions(void)
{
    NodeRegion *r = first_region;
    while(r != NULL) {
        NodeRegion *rp = r;
        for(size_t i = 0; i < rp->count; ++i) {
            free(rp->items[i].links);
        }
        r = r->next;
        free(rp);
    }
}

void reset_all_regions(void)
{
    for(NodeRegion *r = first_region; r != NULL; r = r->next) {
        r->count = 0;
    }
    last_region = first_region;
}

Node *node_alloc(void)
{
    if(first_region == NULL) {
        first_region = malloc(sizeof(NodeRegion));
        memset(first_region, 0, sizeof(*first_region));
        last_region  = first_region;
    }
    while(last_region->count + 1 > NODE_REGION_CAPACITY && last_region->next != NULL) 
        last_region = last_region->next;
    if(last_region->count + 1 > NODE_REGION_CAPACITY) {
        NodeRegion *new = malloc(sizeof(NodeRegion));
        memset(new, 0, sizeof(*new));
        last_region->next = new;
        last_region = new;
    }
    Node *node = &last_region->items[last_region->count++];
    // If we're reusing after reset_all_regions then last_region->next must be not NULL
    // thus we can just memset all the links to be zero
    if(!last_region->next) node->links = malloc(sizeof(Node*) * links_cap);
    else memset(node->links, 0, sizeof(Node*) * links_cap);
    node->links_cap = links_cap;
    node->links_count = 0;
    node->color = COLOR_NODE;

    total_nodes += 1;

    if(first_node) {
        last_node->next = node;
        node->next = first_node;
        last_node = node;
    } else {
        first_node = node;
        last_node = node;
        node->next = node;
    }

    return node;
}

void node_connect(Node *a, Node *b)
{
    for(size_t i = 0; i < a->links_count; ++i) {
        if(a->links[i] == b) return;
    }

    if(a->links_count + 1 < a->links_cap && b->links_count + 1 < b->links_cap)  {
        a->links[a->links_count++] = b;
        b->links[b->links_count++] = a;
    }
    total_links += 1;
}

float calculate_avg_steps(void)
{
    if (!first_node) return 0.0;
    float total = 0.0;
    size_t count = 0;

    Node *node = first_node;
    do {
        node = node->next;
    } while(node != first_node);

    if(count == 0) return 0;
    return total/count;
}

Node *find_nearest_node_within(int x, int y, int radius)
{
    if (!first_node) return NULL;

    Node *nearest = NULL;
    float bestDistSq = (float)(radius * radius);

    Node *node = first_node;
    do {
        float dx = (float)(node->x - x);
        float dy = (float)(node->y - y);
        float distSq = dx*dx + dy*dy;

        if (distSq <= bestDistSq) {
            bestDistSq = distSq;
            nearest = node;
        }

        node = node->next;
    } while (node != first_node); // circular list

    return nearest;
}

void reset_simulation(void)
{
    reset_all_regions();
    first_node = NULL;
    last_node  = NULL;
    total_nodes = 0;
    total_links = 0;
}

#define NODE_RADIUS 10

int main(void)
{
    InitWindow(800, 600, "Small World");

    reset_simulation();
    Node *selected = NULL;
    while(!WindowShouldClose()) {
        ClearBackground(pallete[COLOR_BACKGROUND]);
        BeginDrawing();
        int fps = GetFPS();
        SetWindowTitle(TextFormat("Small World - FPS: %d, Nodes: %zu, Links: %zu, Avg. Steps: %5.2f", 
                    fps, total_nodes, total_links, avg_steps));
        Vector2 cursor = GetMousePosition();
        if(IsKeyPressed(KEY_R)) {
            reset_simulation();
        }
        if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            Node *node = node_alloc();
            node->x = cursor.x;
            node->y = cursor.y;
        }
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Node *node = find_nearest_node_within(cursor.x, cursor.y, NODE_RADIUS);
            if(node) {
                if(selected) {
                    node_connect(selected, node);
                    selected->color = COLOR_NODE;
                    selected = NULL;
                } else {
                    selected = node;
                    selected->color = COLOR_SELECTED_NODE;
                }
            } else {
                if(selected) {
                    selected->color = COLOR_NODE;
                    selected = NULL;
                }
            }
        }

        Node *node = 0;
        for(node = first_node; node != last_node; node = node->next) {
            DrawCircle(node->x, node->y, NODE_RADIUS, pallete[node->color]);
        }
        if(node) DrawCircle(node->x, node->y, NODE_RADIUS, pallete[node->color]);

        for(node = first_node; node != last_node; node = node->next) {
            for(size_t i = 0; i < node->links_count; ++i) {
                Node *link = node->links[i];
                DrawLine(node->x, node->y, link->x, link->y, RED);
            }
        }
        if(node) {
            for(size_t i = 0; i < node->links_count; ++i) {
                Node *link = node->links[i];
                DrawLine(node->x, node->y, link->x, link->y, RED);
            }
        }

        if(calc_total_links != total_links || calc_total_nodes != total_nodes) {
            avg_steps = calculate_avg_steps();
            calc_total_links = total_links;
            calc_total_nodes = total_nodes;
        }

        EndDrawing();
    }
    CloseWindow();
    free_all_regions();
    return 0;
}

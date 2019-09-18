#include "buffer.h"
#include "data_structure.h"

#include <stdio.h>
#include <stdlib.h>

#define RED 0
#define BLACK 1

struct node_s {
  struct node_s *parent;
  struct node_s *left;
  struct node_s *right;
  size_t color;
  char key;
};

node_t *node_init(char key) {
  node_t *n = (node_t *)malloc(sizeof(node_t));
  n->left = n->right = n->parent = NULL;
  n->key = key;
  return n;
}

void node_destroy(node_t *n) { free(n); }

char node_key(node_t *n) { return n->key; }

node_t *node_find(char key, node_t *root) {
  while (root) {
    if (key < root->key)
      root = root->left;
    else if (key > root->key)
      root = root->right;
    else
      return root;
  }
  return NULL;
}

void red_black_insert(node_t *node, node_t **root);

bool node_insert(node_t *node_to_insert, node_t **root) {
  node_t **n = root, *parent = NULL;
  while (*n) {
    parent = *n;
    if (node_to_insert->key < parent->key)
      n = &(parent->left);
    else if (node_to_insert->key > parent->key)
      n = &(parent->right);
    else
      return false;
  }

  node_to_insert->parent = parent;
  *n = node_to_insert;
  red_black_insert(node_to_insert, root);
  return true;
}

typedef struct node_print_item_s {
  size_t position;
  node_t *node;
  bool spacer;
} node_print_item_t;

static int get_depth(node_t *n) {
  int depth = 0;
  while (n) {
    depth++;
    n = n->parent;
  }
  return depth;
}

static int _get_right_depth(node_t *n, int depth) {
  if(n->right)
    depth = _get_right_depth(n->right, depth+1);
  else if(n->left)
    depth = _get_right_depth(n->left, depth);
  return depth;
}

static int get_right_depth(node_t *n) {
  return get_depth(n) + _get_right_depth(n, 0);
}

static node_t *get_root(node_t *n) {
  while(n->parent)
    n = n->parent;
  return n;
}

static int _get_left_depth(node_t *n, int depth) {
  int m = depth;
  if(n->left) {
    int d = _get_left_depth(n->left, depth+1);
    if(d > m)
      m = d;
  }
  if(n->right) {
    int d = _get_left_depth(n->right, depth );
    if(d > m)
      m = d;
  }
  return m;
}

static int get_left_height(node_t *n) {
  return _get_left_depth(n, 0);
}

size_t get_print_length(buffer_t *bh) {
  char *p = buffer_data(bh);
  char *ep = p + buffer_length(bh);
  size_t res = 0;
  while(p < ep) {
    if(*p == 0x1B) {
      p++;
      while(*p && *p != 'm')
        p++;
      if(*p == 'm')
        p++;
    }
    else {
      p++;
      res++;
    }
  }
  return res;
}

void node_print(node_t *root) {
  if (!root)
    return;
  buffer_t *new_nodes_line = buffer_init(100);
  buffer_t *new_slashes_line = buffer_init(100);
  buffer_t *print_items = buffer_init(100);
  buffer_t *next_items = buffer_init(100);

  node_print_item_t item;
  item.position = 0;
  item.node = root;
  item.spacer = false;
  int depth = 0;
  buffer_set(print_items, &item, sizeof(item));
  while (buffer_length(print_items) > 0) {
    // line of text containing node labels
    buffer_clear(new_nodes_line);

    // line of text containing slashes
    buffer_clear(new_slashes_line);

    // for the next loop
    buffer_clear(next_items);
    node_print_item_t *items = (node_print_item_t *)buffer_data(print_items);
    size_t num_items = buffer_length(print_items) / sizeof(node_print_item_t);
    depth++;

    for (size_t i = 0; i < num_items; i++) {
      items[i].spacer = false;
      size_t total = 0;
      for (size_t j = num_items-1; j > i; j--) {
        /* not exactly sure what +2 to +4 does, experimentally found */
        total += get_left_height(items[j].node)+4;
      }
      size_t total2 = get_left_height(get_root(items[i].node));
      if(total && total2 <= total)
        items[i].spacer = true;

      // add leading whitespace
      buffer_appendn(new_nodes_line, ' ',
                     items[i].position - get_print_length(new_nodes_line));
      buffer_appendn(new_slashes_line, ' ',
                     items[i].position - buffer_length(new_slashes_line));

      if (!items[i].spacer) {
        char key = items[i].node->key;
        const char *scode = "";
        const char *ecode = "";
        if(items[i].node->color == RED) {
          scode = "\x1B[31m";
          ecode = "\x1B[0m";
        }
        buffer_appendf(new_nodes_line, "%s%c%d%s", scode, key,
                       get_depth(items[i].node), ecode );
      } else
        buffer_appendc(new_nodes_line, '|');

      if (items[i].spacer || items[i].node->left) {
        node_t *left_node =
            items[i].spacer ? items[i].node : items[i].node->left;
        buffer_appendc(new_slashes_line, '|');
        item.position = items[i].position;
        item.node = left_node;
        item.spacer = items[i].spacer;
        buffer_append(next_items, &item, sizeof(item));
      } else
        buffer_appendc(new_slashes_line, ' ');

      buffer_appendn(new_slashes_line, ' ',
                     get_print_length(new_nodes_line) -
                         buffer_length(new_slashes_line));

      if (!items[i].spacer && items[i].node->right) {
        buffer_appendc(new_slashes_line, '\\');
        item.position = buffer_length(new_slashes_line);
        item.node = items[i].node->right;
        item.spacer = false;
        buffer_append(next_items, &item, sizeof(item));
      } else
        buffer_appendc(new_slashes_line, ' ');
    }
    printf("%s\n", buffer_data(new_nodes_line));
    printf("%s\n", buffer_data(new_slashes_line));
    // swap next_items and print_items
    buffer_t *tmp = print_items;
    print_items = next_items;
    next_items = tmp;
  }
  buffer_destroy(print_items);
  buffer_destroy(next_items);
  buffer_destroy(new_nodes_line);
  buffer_destroy(new_slashes_line);
}

void _debug_node_print(const char *function, int line, node_t *root) {
  printf( "%s:%d\n", function, line );
  node_print(root);
}

#define debug_node_print(x) _debug_node_print(__FUNCTION__, __LINE__, x)

node_t *node_first(node_t *n) {
  if (!n)
    return NULL;
  while (n->left)
    n = n->left;
  return n;
}

node_t *node_last(node_t *n) {
  if (!n)
    return NULL;
  while (n->right)
    n = n->right;
  return n;
}

node_t *node_next(node_t *n) {
  if (n->right) {
    n = n->right;
    while (n->left)
      n = n->left;
    return n;
  }
  while (n->parent && n->parent->right == n)
    n = n->parent;
  return n->parent;
}

bool node_previous_supported() { return true; }

node_t *node_previous(node_t *n) {
  if (n->left) {
    n = n->left;
    while (n->right)
      n = n->right;
    return n;
  }
  while (n->parent && n->parent->left == n)
    n = n->parent;
  return n->parent;
}

static inline node_t *left_deepest_node(node_t *n) {
  while (true) {
    if (n->left)
      n = n->left;
    else if (n->right)
      n = n->right;
    else
      return n;
  }
}

node_t *node_first_to_erase(node_t *n) {
  if (!n)
    return NULL;
  return left_deepest_node(n);
}

node_t *node_next_to_erase(node_t *n) {
  node_t *parent = n->parent;
  if (parent && n == parent->left && parent->right)
    return left_deepest_node(parent->right);
  else
    return parent;
}

/*
 Red–black tree Properties (https://en.wikipedia.org/wiki/Red–black_tree)

1. Each node is either red or black.
2. The root is black. This rule is sometimes omitted. Since the root can always
   be changed from red to black, but not necessarily vice versa, this rule has
   little effect on analysis.
3. All leaves (NIL) are black.
4. If a node is red, then both its children are black.
5. Every path from a given node to any of its descendant NIL nodes contains the
   same number of black nodes.

My additional rules for clarification which are based upon the first 5 rules.
6. If a node has one child, it must be red
7. The parent of a red node must be black
 */

int count_black_nodes(node_t *n) {
  int black_nodes = 0;
  while(n) {
    if(n->color == BLACK)
      black_nodes++;
    n = n->parent;
  }
  return black_nodes;
}

void test_red_black_tree(node_t *root) {
  /* an empty tree is valid */
  if(!root)
    return;
  /* the root is black */
  if(root->color != BLACK) {
    printf( "The root is not black!\n" );
    node_print(root);
    abort();
  }
  node_t *n = node_first(root);
  int black_nodes = 0;
  while(n) {
    /* this is just a general test of binary tree */
    if(n->parent && n->parent->parent == n) {
      printf( "Node(%c) is the same as it's grandparent(%c)\n", n->key, n->parent->parent->key );
      abort();
    }
    /* this is just a general test of binary tree */
    if(n->parent && n->parent->parent && n->parent->parent->parent == n->parent) {
      printf( "Node->Parent(%c) is the same as Node->Parent\'s grandparent\n", n->parent->key);
      abort();
    }
    /* check if one child and that child is red */
    if(n->left && !n->right && n->left->color != RED) {
      printf( "Node(%c) has one left child and it isn't red (%c)\n", n->key, n->left->key );
      node_print(root);
      abort();
    }
    if(!n->left && n->right && n->right->color != RED) {
      printf( "Node(%c) has one right child and it isn't red (%c)\n", n->key, n->right->key );
      node_print(root);
      abort();
    }
    if(n->color == RED) {
      if(n->left && n->left->color == RED) {
        printf( "The red node(%c) has a red left child(%c)\n", n->key, n->left->key );
        node_print(root);
        abort();
      }
      if(n->right && n->right->color == RED) {
        printf( "The red node(%c) has a red right child(%c)\n", n->key, n->right->key );
        node_print(root);
        abort();
      }
      if(n->parent && n->parent->color == RED) {
        printf( "The red node(%c) has a red parent(%c)\n", n->key, n->parent->key );
        node_print(root);
        abort();
      }
    }
    if(!n->left && !n->right) { /* only consider leaf nodes */
      int black_nodes2 = count_black_nodes(n);
      if(!black_nodes)
        black_nodes = black_nodes2;
      if(black_nodes != black_nodes2) {
        printf( "Black node mismatch(%c): %d != %d\n", n->key, black_nodes, black_nodes2 );
        node_print(root);
        abort();
      }
    }
    n = node_next(n);
  }
}

static inline void _swap_nodes(node_t *dest,
                               node_t *src,
                               node_t **root) {
  node_t *parent = src->parent;
  if(parent) {
    if(parent->left == src)
      parent->left = dest;
    else
      parent->right = dest;
    dest->parent = parent;
  }
  else {
    dest->parent = NULL;
    *root = dest;
  }
}

/*
rotate_right
A
| \
B  C
| \
D  E

becomes
B
| \
D  A
   | \
   C  E

In other words,
A:LEFT becomes new root
A's left child becomes the new root's right child
A becomes right child of the new root
The new root's left child remains left child of the new root (nothing)
A's right child remains right child of A (nothing)

Because this has parent linkage, the following also applies
The new_root's parent is set to A's parent
The new_root's parent left or right is modified to reference new_root (as
  opposed to A).
A's parent becomes new_root
A's left child's parent becomes A
*/

void rotate_left(node_t *A, node_t **root) {
  node_t *new_root = A->right;
  _swap_nodes(new_root, A, root);

  node_t *tmp = new_root->left;
  new_root->left = A;
  A->parent = new_root;

  A->right = tmp;
  if(A->right)
    A->right->parent = A;
}

void rotate_right(node_t *A, node_t **root) {
  node_t *new_root = A->left;
  _swap_nodes(new_root, A, root);

  node_t *tmp = new_root->right;
  new_root->right = A;
  A->parent = new_root;

  A->left = tmp;
  if(A->left)
    A->left->parent = A;
}

void rotate_right_and_swap_colors(node_t *A, node_t **root) {
  size_t tmp = A->color;
  A->color = A->left->color;
  A->left->color = tmp;
  rotate_right(A, root);
}

void rotate_left_and_swap_colors(node_t *A, node_t **root) {
  size_t tmp = A->color;
  A->color = A->right->color;
  A->right->color = tmp;
  rotate_left(A, root);
}


void paint_self_red_and_children_black(node_t *n) {
  n->color = RED;
  n->left->color = n->right->color = BLACK;
}

void red_black_insert(node_t *node, node_t **root) {
  /*
    It is assumed that node is linked into it's proper parent and that the
    node is a leaf node.  The red black tree always initially paints the
    given node red.
  */
  node->color = RED;
  node->left = node->right = NULL;
  node_t *parent, *grandparent, *uncle;

  while (1) {
    parent = node->parent;
    if(!parent) {
      /* if the node is the root node, color it black */
      node->parent = NULL;
      node->color = BLACK;
      break;
    }

    /* if the parent is black, the node is red so we're done */
    if(parent->color == BLACK)
      break;
    /* The parent and the node are both red.  It is a violation of
       the red black tree to have two red nodes in a row.

       At this point, the parent must be red and the parent's
       parent would be black as it is a violation of the red
       black tree to have two red nodes in a row.  The red black
       tree insert operation consider's the node's uncle's color.
       The uncle would be the grandparent's other child.
    */
    grandparent = parent->parent;
    if(grandparent->left == parent) {
      /* If the parent is the grandparent's left node, the uncle
         is the right node. */
      uncle = grandparent->right;
      if(uncle && uncle->color == RED) {
        /* If the uncle is red, then the parent and the uncle are
           both red.  The red black tree needs to maintain a
           constant black height.

                   A
                 /   \
                B     C
              /   \
             d     e
            /
           n

           Prior to inserting node (n) which is red, notice that the
           leaf nodes d, e, and C all have a black height of 2 meaning
           that there are only 2 black nodes in the path from the root
           to each of the leaf nodes.  If you recolor d and e black and
           change B to be red, it doesn't change the black height of any
           of the leaf nodes.

                   A
                 /   \
                b     C
              /   \
             D     E
            /
           n

           Notice that in this case, the recoloring created a valid red
           black tree.  The black height is 2 to every leaf node.  The root is
           black.  There are not two red nodes in a row.  In the one case where
           a node only has a single child, the child is red.

           To recap, if the parent and uncle are red, paint the parent and uncle
           black and the grandparent red.  It's possible that the grandparent's
           parent was also red.  To handle this case, we can repeat all of the
           tests recursively (since the recursion is simple, continuing in a
           while loop works by changing the node to the grandparent). */
        grandparent->color = RED;
        parent->color = uncle->color = BLACK;
        node = grandparent;
        continue;
      }
      /* The uncle is black (NULL in this case), then rotate to the right around
         the grandparent.  Notice that while swapping colors during the rotate,
         that the placement of the black node doesn't change. This maintains the
         proper black height.

                 A
               /   \
              B     C
            /
           d
          /
         n

         becomes

               A
             /   \
            D     C
          /   \
         n     b

         using a right rotation around the grandparent (B).

         If n was the right child of the parent, then do an extra left rotation
         to make the tree look like the case above before doing the right
         rotation (there's no need for color swapping because they're both red).

                 A
               /   \
              B     C
            /
           d
            \
             n

         becomes

                 A
               /   \
              B     C
            /
           n
          /
         d

         through a left rotation around the parent (d) and then with a right
         rotation it ultimately becomes.

                 A
               /   \
              N     C
            /   \
           d     b

         using a right rotation around the grandparent (B).
        */
      if(parent->right == node)
        rotate_left(parent, NULL);
      rotate_right_and_swap_colors(grandparent, root);
      break;
    }
    else {
      /* This is the same as the case above where the uncle was the right node,
         except every left is swapped for right and vice versa. */
      uncle = grandparent->left;
      if(uncle && uncle->color == RED) {
        grandparent->color = RED;
        parent->color = uncle->color = BLACK;
        node = grandparent;
        continue;
      }
      if(parent->left == node)
        rotate_right(parent, NULL);
      rotate_left_and_swap_colors(grandparent, root);
      break;
    }
  }
  test_red_black_tree(*root);
}

void node_fix_color(node_t *parent, node_t *node, node_t **root) {
  /*
    When a node is inserted, the red black tree coloring is fixed by looking
    at the node's uncle (the parent's sibling).  When a node is erased or
    removed, the tree coloring is fixed by looking at the color of the node's
    sibling.

    Initially, the node to erase is NULL as it has been unlinked from the tree.
    Only the parent is known and the parent is known to have exactly one node
    which will be the sibling.  Just like with insert where recursion happens
    and when it does, the node to erase may be a valid node.  Below, we will
    consider cases and refer to N as the node even if it is erased.  If a node
    can be either color, upper and lowercase will be shown (Gg) for example.

    If the sibling is red, then it's children and parent must be black.
    Rotating the tree so that the sibling becomes the parent will look like
    the following.

        P
       / \
      N   s
         / \
        A   B

    becomes

          S
         / \
        P   B
       / \
      N   A

    Using this rotation, the new sibling A is black and the process can
    continue.  Also notice that the sibling remains on the same side.

    From this point, the sibling is black, then the tree will look like
    one of the following.  All of the examples will assume that the sibling
    is on the right side.  All of the lefts and rights are swapped if the
    the sibling is on the left side.

    1.  The parent and sibling is black and has no children or both children
        are black.

            Gg           Gg
           /  \         /  \
          Oo   P  =>  Oo    P    => treat g as the parent and P as the node
              / \            \      to find the sibling, If P is left child of
             N   S            s     g, then the sibling will be the right child
                                    of g.  Repeat the process of fixing the
                                    color with g and the current node being P.

          If the sibling is black and has no children, the sibling is colored
          red and the parent is colored black.  If the parent was already black,
          then the parent becomes double black and the process is repeated with
          the parent's parent and the parent.  If the parent's parent is NULL or
          it is red, simply color the parent black.

          sibling->color = RED;
          if(parent->parent && parent->color == BLACK) {
            node = parent;
            parent = parent->parent;
            continue;
          }
          else // the parent is the root or it is red
            parent->color = BLACK;

      2.  The sibling has a red child in the same direction as the sibling (
          ex. The sibling is to the right of the parent and the right child of
          the sibling is red). This is solved by doing a left rotate after
          painting the right node of the sibling black.

            Pp                Ss
           / \               / \
          N   S       =>    P   R
             / \             \
            Ll  r             Ll

       3.  The sibling has a red child in the opposite direction as the sibling.
           This is solved by right rotating the left child around the sibling
           and swapping colors.  Similar to 2, it is finished by doing a left
           rotate after painting the right node of L to be black.

             Pp          Pp              Ll
            /  \         / \            /  \
           N    S  =>   N   L     =>   P    S
               /             \
              l               s
  */

  node_t *sibling;
  if(parent->right != node) {
    sibling = parent->right;
    if(sibling->color == RED) {
      /* by rotating, the sibling will become black which allows for the other
         cases to be tested. */
      rotate_left_and_swap_colors(parent, root);
      sibling = parent->right;
    }
    if(sibling->left) {
      if(sibling->right) {
        if(sibling->right->color == RED) {
          /* the right sibling is black and the right child of sibling
             is red.  Rotate to the left swapping colors with parent and
             parent's right node setting the sibling->right to BLACK
             prior to rotation. */
          sibling->right->color = BLACK;
          rotate_left_and_swap_colors(parent, root);
        }
        else {
          if(sibling->left->color == RED) {
            rotate_right_and_swap_colors(sibling, root);
            rotate_left_and_swap_colors(parent, root);
            sibling->color = BLACK;
          }
          else {
            /* sibling is black and both children are black */
            sibling->color = RED;
            if(parent->parent && parent->color == BLACK)
              node_fix_color(parent->parent, parent, root);
            else
              parent->color = BLACK;
          }
        }
      }
      else { // must be RED because only child right / left
        rotate_right_and_swap_colors(sibling, root);
        rotate_left_and_swap_colors(parent, root);
        sibling->color = BLACK;
      }
    } else if(sibling->right) { // must be red because only child
      sibling->right->color = BLACK;
      rotate_left_and_swap_colors(parent, root);
    } else {
      sibling->color = RED;
      if(parent->parent && parent->color == BLACK)
        node_fix_color(parent->parent, parent, root);
      else
        parent->color = BLACK;
    }
  }
  else {
    sibling = parent->left;
    if(sibling->color == RED) {
      rotate_right_and_swap_colors(parent, root);
      sibling = parent->left;
    }
    if(sibling->right) {
      if(sibling->left) {
        if(sibling->left->color == RED) {
          sibling->left->color = BLACK;
          rotate_right_and_swap_colors(parent, root);
        }
        else {
          if(sibling->right->color == RED) {
            rotate_left_and_swap_colors(sibling, root);
            rotate_right_and_swap_colors(parent, root);
            sibling->color = BLACK;
          }
          else {
            sibling->color = RED;
            if(parent->parent && parent->color == BLACK)
              node_fix_color(parent->parent, parent, root);
            else
              parent->color = BLACK;
          }
        }
      }
      else {
        rotate_left_and_swap_colors(sibling, root);
        rotate_right_and_swap_colors(parent, root);
        sibling->color = BLACK;
      }
    } else if(sibling->left) { // must be red because only child
      sibling->left->color = BLACK;
      rotate_right_and_swap_colors(parent, root);
    } else {
      sibling->color = RED;
      if(parent->parent && parent->color == BLACK)
        node_fix_color(parent->parent, parent, root);
      else
        parent->color = BLACK;
    }
  }
}


void link_child_to_parent(node_t *child, node_t *node, node_t *parent, node_t **root ) {
  if(parent) {
    if(parent->left == node)
      parent->left = child;
    else
      parent->right = child;
  }
  else
    *root = child;
  if(child) {
    child->parent = parent;
    child->color = node->color;
  }
}

bool node_erase(node_t *node, node_t **root) {
  // node_print(*root);
  /* In a red black tree, a primary concern is to always maintain the black
     height.

     1.  If the node has no children and no parent, then this is the last node
         in the tree and *root is set to NULL.

     2.  If a node has a single child, then that child must be red and the
         given node to erase must be black.  This is the simplest case in that
         you only need to swap the red child of the node to erase with the node
         to erase by linking the red child to the node to erase's parent.  To
         maintain the black height, the red child's color will change to black.

         A
          \
           b

         becomes

         B

     3.  If the node has no children or the node has two black children, then
         unlink the node from it's parent.  If the node itself was black, this
         causes the parent to be double-black and requires that the color be
         fixed.  A node is considered double-black if in the process of removing
         the node, you lose the ability to maintain a constant black height.
         In the process of removing a node, there will be at most one
         double-black node, so that node can be fixed through rotations and
         recoloring (to be discussed in a bit).

            A           A (double-black)
           / \    =>   /
          B   N       B

          Imagine if we removing N, then B would be the only child of A so B
          must be red which would cause the rest of the tree to not necessarily
          maintain a constant black height.  Instead of changing B to red, A is
          marked double black and then the color is fixed.

     To erase a node in a binary tree when a node has two children, you must
     find the successor.  The successor is defined as the leftmost node to the
     right child.  One reasonably obvious feature of choosing the successor to
     replace the node to erase is that it will not have a left child.  This
     makes the replacement easy in as much as you can link the node to erase's
     left child to the successor without having to worry about it's previous
     left child (since it doesn't have one).  This presents four scenarios
     (numbered 4a, 4b, 5a, 5b).

     4.  If the right child of the node doesn't have a left child, then the
         right child is the successor.  In this case, the successor is promoted
         it's parent (the node to erase).  The color of the successor and the
         node to erase are swapped (so during the swap if the node to erase was
         red, then the successor will become red and vice versa).

         a) If there is a right child to the successor, then the successor must
            be black (because the right child is an only child and must be red).

          N
           \
            S
             \
              sR (successor right child)

          becomes

          S
           \
            SR

          The other case here is that the node to erase is red.  In this case,
          the following happens.

          n
           \
            S
             \
              sR (successor right child)

          becomes

          s
           \
            SR

          In both cases, the black height is maintained.  In both cases,
          color is maintained at the given tree height.  The top node stays
          the color that it was and the successor's right child becomes black.


         N
          \
           s

         becomes

         S

         N
          \
           s
            \
             sR (successor right child)

         becomes

         S
          \
           SR

         In both cases, black height is maintained.

         b) If the node to erase is red and the successor does not have a right
         child, the successor becomes double-black and needs fixed as we have
         lost one black node in the tree.

         n
          \
           S

         becomes

         n

         with a black node being removed, so n must be fixed.

         If the node to erase was black, the following would happen.

         N
          \
           S

         becomes

         N

         again resulting in a black node being removed, so N must be fixed.

     5.  If the right child of the node does have a left child, traverse the
         left nodes until a node without a left node is found.  This is the
         successor.  The successor will have at most one right child which
         will be red by definition (because any single child is red).

         a) The successor has a right child.  In this case, the successor is
         removed from the tree like case 2 above by simply linking the leaf
         node (successor's right child to the successor's parent).  Once this
         is done, the successor will replace the node to erase.  It will take on
         the node to erase's left, right, parent, and color, and link the nodes
         to the left, right, and parent to the successor node instead of the
         node to erase.

             N (or n)
              \
               A
              /
             B
            /
           C
            \
             d

          becomes

              N (or n)
               \
                A
               /
              B
             /
            D

          with C removed

          The C and N or n are swapped.

             C (or c)
              \
               A
              /
             B
            /
           D

          b) The successor does not have a right child (or any children).  If C
          is black, then the black height will change when C is taken out and
          put in place of N.

              N (or n)
               \
                A
               /
              B
             /
            C

          becomes
              C (or c)
               \
                A
               /
              B

          B is double-black and needs fixed since C was black.  If C was red,
          then B will be black and okay.  The removal of C from B is like
          case 3 above where fixing the parent node (B) is necessary if B's
          child (C) is black.
  */
  node_t *parent = node->parent;
  if(!node->left) {
    if(node->right) { /* 2. node has one red right child */
      link_child_to_parent(node->right, node, parent, root);
    }
    else { /* 3. node has no children, parent becomes double black if node is black */
      if(parent) {
        if(parent->left == node)
          parent->left = NULL;
        else
          parent->right = NULL;
        if(node->color == BLACK)
          node_fix_color(parent, NULL, root);
      }
      else { /* 1.  no children, no parent, tree is now empty */
        *root = NULL;
      }
    }
  } else if(!node->right) { /* 2. node has one red left child */
    link_child_to_parent(node->left, node, parent, root );
  } else { /* node has left and right child */
    /* swap the successor with the current node maintaining the color of the
       current node in the successor.  The successor will only have a right
       child or no child at all.  If the successor only has a right child, then
       this will be similar to 1 (if successor is to the right of the current
       node, then only paint the successor's right child black and swap the
       current node with the successor).  Otherwise, if the successor is black,
       the successor's previous parent will become double black. If the successor
       is to the right of the current node */

    node_t *successor = node->right;
    if(!successor->left) {
      node_t tmp = *successor;
      link_child_to_parent(successor, node, parent, root);
      successor->left = node->left;
      successor->left->parent = successor;
      if(successor->right) // 4a
        successor->right->color = BLACK;
      else { // 4b
        if(tmp.color == BLACK)
          node_fix_color(successor, NULL, root);
      }
    }
    else {
      while(successor->left)
        successor = successor->left;

      if(successor->right) { // 5a
        link_child_to_parent(successor->right, successor, successor->parent, root);

        /* Replace node with successor (left, right, parent, color) */
        link_child_to_parent(successor, node, parent, root);
        successor->left = node->left;
        successor->left->parent = successor;
        successor->right = node->right;
        successor->right->parent = successor;
      }
      else { // 5b
        node_t tmp = *successor;

        link_child_to_parent(successor, node, parent, root);
        successor->left = node->left;
        successor->left->parent = successor;
        successor->right = node->right;
        successor->right->parent = successor;

        parent = tmp.parent;
        if(parent->left == successor)
          parent->left = NULL;
        else
          parent->right = NULL;
        if(tmp.color == BLACK)
          node_fix_color(parent, NULL, root);
      }
    }
  }
  test_red_black_tree(*root);
  return true;
}

#include <stdio.h>

int main() {

    node btree;

    func7(current, input_number)
}

typedef struct {

    int value;
    node *left;
    node *right;
    int padding;

} node;

int func7(node *current, int user_input) {
    // Check for Null
    if (current == NULL) {
        return -1;
    }

    // current value has to be <= input
    if (current->value <= user_input) {
        // Value cannot = input
        if (current->value == user_input) {
            return 0;
        }

        current = current->right;
        return func7(current, user_input) * 2 + 1;
    }
    // Go left
    current = current->left;
    return func7(current, user_input) * 2;
}
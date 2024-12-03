// Private variable
static int shared_variable = 0;

// Public interface
void set_shared_value(int value) {
    shared_variable = value;
}

int get_shared_value(void) {
    return shared_variable;
} 

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define MAX_STRING_LENGTH 1024
#define MAX_TOKENS 1024

typedef enum {
    VALUE,
    OPERATOR,
    LEFT_BRACKET,
    RIGHT_BRACKET,
} TokenKind;

typedef struct {
    char *start_position;
    char *end_position;
    TokenKind kind;
    float value;
} Token;

typedef struct {
    Token _items[MAX_TOKENS];
    int count;
} TokenStack;

void print_token(Token token)
{
    if (token.kind == VALUE)
    {
        printf("VALUE (%f) ", token.value);
    }
    else
    {
        printf("OPERATOR (%c) ", *token.start_position);
    }
}

void print_token_stack(TokenStack token_stack)
{
    for (int i = 0; i < token_stack.count; i += 1)
    {
        Token token = token_stack._items[i];
        print_token(token);
    }

    printf("\n");
}

int get_operator_precedence(char c)
{
    if (c == '+') return 1;
    if (c == '-') return 1;
    if (c == '*') return 2;
    if (c == '/') return 2;
    if (c == '^') return 3;

    return 0;
}

void push(TokenStack *token_stack, Token token)
{
    token_stack->_items[token_stack->count] = token;
    token_stack->count += 1;
}

Token pop(TokenStack *token_stack)
{
    Token nothing = {0};
    if (token_stack->count < 1)
    {
        printf("No values to pop.\n");
        return nothing;
    }

    Token result = token_stack->_items[token_stack->count-1];
    token_stack->count -= 1;
    return result;
}

Token peek(TokenStack *token_stack)
{
    return token_stack->_items[token_stack->count-1];
}

float evaluate(Token *token_buffer, int token_count)
{
    // Build RPN form
    TokenStack output = {0};
    TokenStack operators = {0};

    for (int i = 0; i < token_count; i += 1)
    {
        Token token = token_buffer[i];

        if (token.kind == VALUE)
        {
            push(&output, token);
        }
        else if (token.kind == OPERATOR)
        {
            int precedence = get_operator_precedence(*token.start_position);

            if (operators.count > 0)
            {
                Token peeked_operator = peek(&operators);
                int peeked_precedence = get_operator_precedence(*peeked_operator.start_position);
                if (precedence < peeked_precedence)
                {
                    // Move all higher or equal operators to the output
                    while (precedence <= peeked_precedence)
                    {
                        Token popped_operator = pop(&operators);
                        push(&output, popped_operator);

                        if (operators.count > 0)
                        {
                            peeked_operator = peek(&operators);
                            peeked_precedence = get_operator_precedence(*peeked_operator.start_position);
                        }
                        else
                        {
                            break;
                        }
                    }
                }

            }

            push(&operators, token);
        }
        else if (token.kind == LEFT_BRACKET)
        {
            push(&operators, token);
        }
        else if (token.kind == RIGHT_BRACKET)
        {
            // Push all operators onto the output until we reach the left bracket
            bool stop = false;
            while (!stop)
            {
                if (operators.count > 0)
                {
                    Token peeked_operator = peek(&operators);
                    if (peeked_operator.kind != LEFT_BRACKET)
                    {
                        Token popped_operator = pop(&operators);
                        push(&output, popped_operator);
                    }
                    else
                    {
                        (void)pop(&operators);
                        stop = true;
                    }
                }
                else
                {
                    printf("Mismatched parenthesis.\n");
                    return 0;
                }
            }
        }
    }

    // Move all remaining operator tokens to the output
    while (operators.count > 0)
    {
        Token operator = pop(&operators);
        push(&output, operator);
    }

    // Print the resulting output stack
    // print_token_stack(output);

    // Evaluate the stack
    TokenStack value_stack = {0};
    for (int i = 0; i < output.count; i += 1)
    {
        Token token = output._items[i];
        if (token.kind == VALUE)
        {
            push(&value_stack, token);
        }
        else
        {
            if (value_stack.count < 2)
            {
                printf("Not enough values provided for operator \"%c\"\n", *token.start_position);
                return 0.0f;
            }

            Token lvalue = pop(&value_stack);
            Token rvalue = pop(&value_stack);
            char operator = *token.start_position;

            float result = 0.0f;
            if (operator == '+')
            {
                result = lvalue.value + rvalue.value;
            }
            else if (operator == '-')
            {
                // Is this the right order?
                result = rvalue.value - lvalue.value;
            }
            else if (operator == '*')
            {
                result = lvalue.value * rvalue.value;
            }
            else if (operator == '/')
            {
                result = rvalue.value / lvalue.value;
            }
            else if (operator == '^')
            {
                result = powf(rvalue.value, lvalue.value);
            }

            Token result_token = (Token){
                .value = result,
            };

            push(&value_stack, result_token);
        }
    }

    // At the end, there should be just one value left in the stack.
    float result = 0.0f;
    if (value_stack.count > 1)
    {
        printf("Something went wrong! Expected 1 value left on stack after evaluation, got %d.\n", value_stack.count);
    }
    else
    {
        Token result_token = pop(&value_stack);
        result = result_token.value;
    }

    return result;
}

bool is_bracket(char c)
{
    return (c == '(' || c == ')');
}

bool is_start_of_operator(char c)
{
    char operators[] = {
        '+',
        '-',
        '*',
        '/',
        '^',
    };

    for (int i = 0; i < (sizeof(operators)/sizeof(char)); i += 1)
    {
        if (c == operators[i]) 
        {
            return true;
        }
    }

    return false;
}

bool is_start_of_float(char c)
{
    return (('0' <= c && c <= '9') || (c == '.'));
}

bool is_whitespace(char c)
{
    char whitespace[] = {
        ' ',
        '\t',
        '\n',
        '\r',
    };

    for (int i = 0; i < (sizeof(whitespace)/sizeof(char)); i += 1)
    {
        if (c == whitespace[i]) 
        {
            return true;
        }
    }

    return false;
}

int tokenize(char *input_buffer, Token *token_buffer)
{
    char *cursor = input_buffer;
    Token *next_token = token_buffer;
    int token_count = 0;
    while (*cursor != '\0')
    {
        if (is_start_of_float(*cursor))
        {
            char *end;
            float value = strtof(cursor, &end);
            int size = (int)(end - cursor); 

            *next_token = (Token){
                .start_position = cursor,
                .end_position = cursor+size,
                .kind = VALUE,
                .value = value,
            };

            next_token += 1;
            token_count += 1;
            cursor += size;
        }
        else if (is_start_of_operator(*cursor))
        {
            *next_token = (Token){
                .start_position = cursor,
                .end_position = cursor,
                .kind = OPERATOR,
            };

            next_token += 1;
            token_count += 1;
            cursor += 1;
        }
        else if (is_bracket(*cursor))
        {
            char bracket = *cursor;

            TokenKind kind = (bracket == '(' ? LEFT_BRACKET : RIGHT_BRACKET);

            *next_token = (Token){
                .start_position = cursor,
                .end_position = cursor,
                .kind = kind,
            };

            next_token += 1;
            token_count += 1;
            cursor += 1;
        }
        else if (is_whitespace(*cursor))
        {
            while (is_whitespace(*cursor))
            {
                cursor += 1;
            }
        }
        else
        {
            printf("Invalid character! What is a \"%c\"(%d)?", *cursor, *cursor);
            return token_count;
        }
    }

    return token_count;
}

float evaluate_expression(char *s)
{
    Token token_buffer[MAX_TOKENS];

    int token_count = tokenize(s, token_buffer);
    float result = evaluate(token_buffer, token_count);
    return result;
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    char input_buffer[MAX_STRING_LENGTH];
    bool quit = false;
    while (!quit)
    { 
        printf("> ");
        fgets(input_buffer, sizeof(input_buffer), stdin);
        float result = evaluate_expression(input_buffer);
        printf("%g\n", result);
    }

    return 0;
}


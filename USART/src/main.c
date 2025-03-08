//GPIOD ports
#define GPIOD_BASE 0x40020C00
#define GPIOD_MODER ((volatile unsigned int*) (GPIOD_BASE + 0x00))
#define GPIOD_AFRL ((volatile unsigned int*) (GPIOD_BASE + 0x20))
#define GPIOD_AFRH ((volatile unsigned int*) (GPIOD_BASE + 0x24))

#define SCB_VTOR ((volatile unsigned int*)0xE000ED08)

// USART data structure
typedef struct tag_usart {
    volatile unsigned short sr;
    volatile unsigned short Unused0;
    volatile unsigned short dr;
    volatile unsigned short Unsused1;
    volatile unsigned short brr;
    volatile unsigned short Unused2;
    volatile unsigned short cr1;
    volatile unsigned short Unused3;
    volatile unsigned short cr2;
    volatile unsigned short Unused4;
    volatile unsigned short cr3;
    volatile unsigned short Unused5;
    volatile unsigned short gtpr;
} USART;

// USART1
#define USART1 ((USART*) 0x40011000)
#define NVIC_USART1_ISER ((volatile unsigned int*) 0xE000E104)
#define NVIC_USART1_IRQ_BPOS (1 << 5)
#define USART1_IRQVEC ((void (**) (void)) 0x2001C0D4)

// USART2
#define USART2 ((USART*) 0x40004400)
#define NVIC_USART2_ISER ((volatile unsigned int*) 0xE000E104)
#define NVIC_USART2_IRQ_BPOS (1 << 6)
#define USART2_IRQVEC ((void (**) (void)) 0x2001C0D8)

// BIT macrodefinitions
#define BIT_UE (1 << 13)
#define BIT_TE (1 << 3)
#define BIT_RE (1 << 2)
#define BIT_RXNE (1 << 5)
#define BIT_TXE (1 << 7)
#define BIT_EN (1 << 13)
#define BIT_RXNEIE (1 << 5)

// RCC definitions
#define RCC_BASE 0x40023800
#define RCC_APB1RSTR ((volatile unsigned int*) (RCC_BASE + 0x20))
#define RCC_APB1ENR ((volatile unsigned int*) (RCC_BASE + 0x40))
#define RCC_AHB1ENR ((volatile unsigned int*) (RCC_BASE + 0x30))
#define RCC_APB1RSTR_USART2RST (1 << 17) // bit 17
#define RCC_APB1ENR_USART2EN (1 << 17) // bit 17
#define RCC_AHB1ENR_GPIODEN (1 << 3) // bit 3

// Global variables
unsigned char inbuf1;
unsigned char inbuf2;

///////////////////////////////////////////////////////////////////////////////// USART 1 ////////////////////////////////////////////////////////////////////////////////////

void usart1_irq_routine(void) {
    if (USART1->sr & BIT_RXNE) {
        inbuf1 = (char) USART1->dr;
    }
}

char usart1_tstchar(void) {
    char c = inbuf1;
    inbuf1 = 0;
    return c;
}

void usart1_init(void) {
    inbuf1 = 0;
    *USART1_IRQVEC = usart1_irq_routine;
    *NVIC_USART1_ISER |= NVIC_USART1_IRQ_BPOS;
    USART1->brr = 0x2D9;
    USART1->cr3 = 0;
    USART1->cr2 = 0;
    USART1->cr1 = BIT_EN | BIT_RXNEIE | BIT_TE | BIT_RE;
}

void usart1_outchar(char c) {
    while ((USART1->sr & BIT_TXE) == 0);
    USART1->dr = (unsigned short) c;
}

///////////////////////////////////////////////////////////////////////////////// USART 2 ////////////////////////////////////////////////////////////////////////////////////

void usart2_irq_routine(void) {
    if (USART2->sr & BIT_RXNE) {
        inbuf2 = (char) USART2->dr;
    }
}

char usart2_tstchar(void) {
    char c = inbuf2;
    inbuf2 = 0;
    return c;
}
/* Not suitable for MD407 board
void usart2_init(void) {

    inbuf2 = 0;
    *USART2_IRQVEC = usart2_irq_routine;
    *NVIC_USART2_ISER |= NVIC_USART2_IRQ_BPOS;
    USART2->brr = 0x2D9;
    USART2->cr3 = 0;
    USART2->cr2 = 0;
    USART2->cr1 = BIT_EN | BIT_RXNEIE | BIT_TE | BIT_RE;
}
*/ 
void usart2_outchar(char c) {
    while ((USART2->sr & BIT_TXE) == 0);
    USART2->dr = (unsigned short) c;
}

//////////////////////////////////////////////////////////////////////////////// USART2 for MD407 board ///////////////////////////////////////////////////////////////////////////////

// Initialise usart 2 on MD407 board

void usart2_init(void) {

    // AHB1 bus, activate clock for D port
    *RCC_AHB1ENR |= RCC_AHB1ENR_GPIODEN;

    // PD5 and PD6 as alternate function
    *GPIOD_MODER &= ~0xFFFFFFFF; 
    *GPIOD_MODER |= 0x00002800; 

    // alternate function 7 for PD5 and PD6
    *GPIOD_AFRL &= ~0xFFFFFFFF; 
    *GPIOD_AFRL |= (7 << (5 * 4)) | (7 << (6 * 4)); 

    // Start clock
    *RCC_APB1ENR |= RCC_APB1ENR_USART2EN;
    
    // Reset clock
    *RCC_APB1RSTR |= RCC_APB1RSTR_USART2RST;
    *RCC_APB1RSTR &= ~RCC_APB1RSTR_USART2RST;

    inbuf2 = 0;
    *USART2_IRQVEC = usart2_irq_routine;

    *NVIC_USART2_ISER |= NVIC_USART2_IRQ_BPOS;
    USART2->brr = 0x16D; // USARTDIV = 42 * 10^6 / (115200 * 16) = 0x16D, match APB2 frequency 
    USART2->cr3 = 0;
    USART2->cr2 = 0;
    USART2->cr1 = BIT_EN | BIT_RXNEIE | BIT_TE | BIT_RE;
}

///////////////////////////////////////////////////////////////////////////////// Test USART1 ////////////////////////////////////////////////////////////////////////////////////
/*
 // testa USART1
void main(void)
{   
    *SCB_VTOR = 0x2001C000;
   usart1_init(); // Initialise USART1
   while(inbuf1 == 0); // Waits for input in terminal
   usart1_outchar(inbuf1); // Output char in terminal
} 
*/
///////////////////////////////////////////////////////////////////////////////// MAIN ////////////////////////////////////////////////////////////////////////////////////

void main(void) {
    *SCB_VTOR = 0x2001C000;
    usart1_init();
    usart2_init();
    
    while(1) {
        if (inbuf1 != 0) {
            usart2_outchar(inbuf1);
            inbuf1 = 0;
        }
        if (inbuf2 != 0) {
            usart1_outchar(inbuf2);
            inbuf2 = 0;
        }
    }
}
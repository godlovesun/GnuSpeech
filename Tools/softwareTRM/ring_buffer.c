#define BUFFER_SIZE 1024

typedef struct _TRMRingBuffer {
    double buffer[BUFFER_SIZE];
    int padSize;
    int fillSize; // Derived from BUFFER_SIZE and padSize.  Remains constant.

    int fillPtr;
    int emptyPtr;
    int fillCounter;

    void *context;
    void (*callbackFunction)(TRMRingBuffer *, void *);
} TRMRingBuffer;

TRMRingBuffer *createRingBuffer(int aPadSize)
{
    TRMRingBuffer *newRingBuffer;
    int index;

    newRingBuffer = (TRMRingBuffer *)malloc(sizeof(TRMRingBuffer));
    if (newRingBuffer == NULL) {
        fprintf(stderr, "Failed to malloc() space for ring buffer.\n");
        return NULL;
    }

    for (index = 0; index < BUFFER_SIZE; index++)
        buffer[index] = 0;

    padSize = aPadSize;
    fillSize = BUFFER_SIZE - (2 * padSize);

    fillPtr = padSize;
    emptyPtr = 0;
    fillCounter = 0;

    context = NULL;
    callbackFunction = NULL;

    return newRingBuffer;
}

// Fills the ring buffer with a single sample, increments
// the counters and pointers, and empties the buffer when
// full.
void dataFill(TRMRingBuffer *ringBuffer, double data)
{
    buffer[fillPtr] = data;

    /*  INCREMENT THE FILL POINTER, MODULO THE BUFFER SIZE  */
    increment(ringBuffer);

    /*  INCREMENT THE COUNTER, AND EMPTY THE BUFFER IF FULL  */
    if (++fillCounter >= fillSize) {
	dataEmpty(ringBuffer);
	/* RESET THE FILL COUNTER  */
	fillCounter = 0;
    }
}

void dataEmpty(TRMRingBuffer *ringBuffer)
{
    if (callbackFunction == NULL) {
        // Just empty the buffer.
    } else {
        (*callbackFunction)(ringBuffer, context);
    }
}

void increment(TRMRingBuffer *ringBuffer)
{
    if (++(ringBuffer->fillPtr) >= BUFFER_SIZE)
	ringBuffer->fillPtr -= BUFFER_SIZE;
}

void decrement(TRMRingBuffer *ringBuffer)
{
    if (--(ringBuffer->fillPtr) < 0)
	ringBuffer->fillPtr += BUFFER_SIZE;
}

// Pads the buffer with zero samples, and flushes it by converting the remaining samples.
void flushBuffer(TRMRingBuffer *ringBuffer)
{
    int index;

    /*  PAD END OF RING BUFFER WITH ZEROS  */
    for (index = 0; index < (padSize * 2); index++)
	dataFill(ringBuffer, 0.0);

    /*  FLUSH UP TO FILL POINTER - PADSIZE  */
    dataEmpty(ringBuffer);
}

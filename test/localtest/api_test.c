/**********************************************************************************************************************/
/**
 * @file            api_test.c
 *
 * @brief           TRDP test functions on dual interface
 *
 * @details         Extensible test suite working on multihoming / dual interface. Basic functionality and
 *                  regression tests can easily be appended to an array.
 *                  This code is work in progress and can be used verify changes additionally to the standard
 *                  PD and MD tests.
 *
 * @note            Project: TRDP
 *
 * @author          Bernd Loehr
 *
 * @remarks         Copyright NewTec GmbH, 2017.
 *                  All rights reserved.
 *
 * $Id: api_test.c 41539 2016-03-11 18:01:25Z bloehr $
 *
 */


/***********************************************************************************************************************
 * INCLUDES
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined (POSIX)
#include <unistd.h>
#include <sys/select.h>
#elif defined (WIN32)
#include "getopt.h"
#endif

#include "trdp_if_light.h"
#include "vos_sock.h"


/***********************************************************************************************************************
 * DEFINITIONS
 */
#define APP_VERSION  "1.0"

typedef int (test_func_t)(int argc, char *argv[]);

UINT32          gDestMC = 0xEF000101u;
int             gFailed;

static FILE     *gFp    = NULL;

typedef struct
{
    TRDP_APP_SESSION_T  appHandle;
    TRDP_IP_ADDR_T      ifaceIP;
    int                 threadRun;
    VOS_THREAD_T        threadId;
    
} TRDP_THREAD_SESSION_T;

TRDP_THREAD_SESSION_T gSession1 = {NULL, 0x0A000264u, 0, 0};
TRDP_THREAD_SESSION_T gSession2 = {NULL, 0x0A000265u, 0, 0};

/* Data buffers to play with (Content is borrowed from Douglas Adams, "The Hitchhiker's Guide to the Galaxy") */
static uint8_t dataBuffer1[64*1024] = {
    
    "Far out in the uncharted backwaters of the unfashionable end of the western spiral arm of the Galaxy lies a small unregarded yellow sun. Orbiting this at a distance of roughly ninety-two million miles is an utterly insignificant little blue green planet whose ape-descended life forms are so amazingly primitive that they still think digital watches are a pretty neat idea.\n"
    "This planet has – or rather had – a problem, which was this: most of the people on it were unhappy for pretty much of the time. Many solutions were suggested for this problem, but most of these were largely concerned with the movements of small green pieces of paper, which is odd because on the whole it wasn’t the small green pieces of paper that were unhappy.\n"
    "And so the problem remained; lots of the people were mean, and most of them were miserable, even the ones with digital watches.\n"
    "Many were increasingly of the opinion that they’d all made a big mistake in coming down from the trees in the first place. And some said that even the trees had been a bad move, and that no one should ever have left the oceans.\n"
    "And then, one Thursday, nearly two thousand years after one man had been nailed to a tree for saying how great it would be to be nice to people for a change, one girl sitting on her own in a small cafe in Rickmansworth suddenly realized what it was that had been going wrong all this time, and she finally knew how the world could be made a good and happy place. This time it was right, it would work, and no one would have to get nailed to anything.\n"
    "Sadly, however, before she could get to a phone to tell anyone about it, a terribly stupid catastrophe occurred, and the idea was lost forever.\n"
    "This is not her story.\n"
    "But it is the story of that terrible stupid catastrophe and some of its consequences.\n"
    "It is also the story of a book, a book called The Hitchhiker’s Guide to the Galaxy – not an Earth book, never published on Earth, and until the terrible catastrophe occurred, never seen or heard of by any Earthman.\n"
    "Nevertheless, a wholly remarkable book.\n"
    "In fact it was probably the most remarkable book ever to come out of the great publishing houses of Ursa Minor – of which no Earthman had ever heard either.\n"
    "Not only is it a wholly remarkable book, it is also a highly successful one – more popular than the Celestial Home Care Omnibus, better selling than Fifty More Things to do in Zero Gravity, and more controversial than Oolon Colluphid’s trilogy of philosophical blockbusters Where God Went Wrong, Some More of God’s Greatest Mistakes and Who is this God Person Anyway?\n"
    "In many of the more relaxed civilizations on the Outer Eastern Rim of the Galaxy, the Hitchhiker’s Guide has already supplanted the great Encyclopedia Galactica as the standard repository of all knowledge and wisdom, for though it has many omissions and contains much that is apocryphal, or at least wildly inaccurate, it scores over the older, more pedestrian work in two important respects.\n"
    "First, it is slightly cheaper; and secondly it has the words Don’t Panic inscribed in large friendly letters on its cover.\n"
    "But the story of this terrible, stupid Thursday, the story of its extraordi- nary consequences, and the story of how these consequences are inextricably intertwined with this remarkable book begins very simply.\n"
    "It begins with a house.\n"
    "Far out in the uncharted backwaters of the unfashionable end of the western spiral arm of the Galaxy lies a small unregarded yellow sun. Orbiting this at a distance of roughly ninety-two million miles is an utterly insignificant little blue green planet whose ape-descended life forms are so amazingly primitive that they still think digital watches are a pretty neat idea.\n"
    "This planet has – or rather had – a problem, which was this: most of the people on it were unhappy for pretty much of the time. Many solutions were suggested for this problem, but most of these were largely concerned with the movements of small green pieces of paper, which is odd because on the whole it wasn’t the small green pieces of paper that were unhappy.\n"
    "And so the problem remained; lots of the people were mean, and most of them were miserable, even the ones with digital watches.\n"
    "Many were increasingly of the opinion that they’d all made a big mistake in coming down from the trees in the first place. And some said that even the trees had been a bad move, and that no one should ever have left the oceans.\n"
    "And then, one Thursday, nearly two thousand years after one man had been nailed to a tree for saying how great it would be to be nice to people for a change, one girl sitting on her own in a small cafe in Rickmansworth suddenly realized what it was that had been going wrong all this time, and she finally knew how the world could be made a good and happy place. This time it was right, it would work, and no one would have to get nailed to anything.\n"
    "Sadly, however, before she could get to a phone to tell anyone about it, a terribly stupid catastrophe occurred, and the idea was lost forever.\n"
    "This is not her story.\n"
    "But it is the story of that terrible stupid catastrophe and some of its consequences.\n"
    "It is also the story of a book, a book called The Hitchhiker’s Guide to the Galaxy – not an Earth book, never published on Earth, and until the terrible catastrophe occurred, never seen or heard of by any Earthman.\n"
    "Nevertheless, a wholly remarkable book.\n"
    "In fact it was probably the most remarkable book ever to come out of the great publishing houses of Ursa Minor – of which no Earthman had ever heard either.\n"
    "Not only is it a wholly remarkable book, it is also a highly successful one – more popular than the Celestial Home Care Omnibus, better selling than Fifty More Things to do in Zero Gravity, and more controversial than Oolon Colluphid’s trilogy of philosophical blockbusters Where God Went Wrong, Some More of God’s Greatest Mistakes and Who is this God Person Anyway?\n"
    "In many of the more relaxed civilizations on the Outer Eastern Rim of the Galaxy, the Hitchhiker’s Guide has already supplanted the great Encyclopedia Galactica as the standard repository of all knowledge and wisdom, for though it has many omissions and contains much that is apocryphal, or at least wildly inaccurate, it scores over the older, more pedestrian work in two important respects.\n"
    "First, it is slightly cheaper; and secondly it has the words Don’t Panic inscribed in large friendly letters on its cover.\n"
    "But the story of this terrible, stupid Thursday, the story of its extraordi- nary consequences, and the story of how these consequences are inextricably intertwined with this remarkable book begins very simply.\n"
    "It begins with a house.\n"
    "Far out in the uncharted backwaters of the unfashionable end of the western spiral arm of the Galaxy lies a small unregarded yellow sun. Orbiting this at a distance of roughly ninety-two million miles is an utterly insignificant little blue green planet whose ape-descended life forms are so amazingly primitive that they still think digital watches are a pretty neat idea.\n"
    "This planet has – or rather had – a problem, which was this: most of the people on it were unhappy for pretty much of the time. Many solutions were suggested for this problem, but most of these were largely concerned with the movements of small green pieces of paper, which is odd because on the whole it wasn’t the small green pieces of paper that were unhappy.\n"
    "And so the problem remained; lots of the people were mean, and most of them were miserable, even the ones with digital watches.\n"
    "Many were increasingly of the opinion that they’d all made a big mistake in coming down from the trees in the first place. And some said that even the trees had been a bad move, and that no one should ever have left the oceans.\n"
    "And then, one Thursday, nearly two thousand years after one man had been nailed to a tree for saying how great it would be to be nice to people for a change, one girl sitting on her own in a small cafe in Rickmansworth suddenly realized what it was that had been going wrong all this time, and she finally knew how the world could be made a good and happy place. This time it was right, it would work, and no one would have to get nailed to anything.\n"
    "Sadly, however, before she could get to a phone to tell anyone about it, a terribly stupid catastrophe occurred, and the idea was lost forever.\n"
    "This is not her story.\n"
    "But it is the story of that terrible stupid catastrophe and some of its consequences.\n"
    "It is also the story of a book, a book called The Hitchhiker’s Guide to the Galaxy – not an Earth book, never published on Earth, and until the terrible catastrophe occurred, never seen or heard of by any Earthman.\n"
    "Nevertheless, a wholly remarkable book.\n"
    "In fact it was probably the most remarkable book ever to come out of the great publishing houses of Ursa Minor – of which no Earthman had ever heard either.\n"
    "Not only is it a wholly remarkable book, it is also a highly successful one – more popular than the Celestial Home Care Omnibus, better selling than Fifty More Things to do in Zero Gravity, and more controversial than Oolon Colluphid’s trilogy of philosophical blockbusters Where God Went Wrong, Some More of God’s Greatest Mistakes and Who is this God Person Anyway?\n"
    "In many of the more relaxed civilizations on the Outer Eastern Rim of the Galaxy, the Hitchhiker’s Guide has already supplanted the great Encyclopedia Galactica as the standard repository of all knowledge and wisdom, for though it has many omissions and contains much that is apocryphal, or at least wildly inaccurate, it scores over the older, more pedestrian work in two important respects.\n"
    "First, it is slightly cheaper; and secondly it has the words Don’t Panic inscribed in large friendly letters on its cover.\n"
    "But the story of this terrible, stupid Thursday, the story of its extraordi- nary consequences, and the story of how these consequences are inextricably intertwined with this remarkable book begins very simply.\n"
    "It begins with a house.\n"
    "Far out in the uncharted backwaters of the unfashionable end of the western spiral arm of the Galaxy lies a small unregarded yellow sun. Orbiting this at a distance of roughly ninety-two million miles is an utterly insignificant little blue green planet whose ape-descended life forms are so amazingly primitive that they still think digital watches are a pretty neat idea.\n"
    "This planet has – or rather had – a problem, which was this: most of the people on it were unhappy for pretty much of the time. Many solutions were suggested for this problem, but most of these were largely concerned with the movements of small green pieces of paper, which is odd because on the whole it wasn’t the small green pieces of paper that were unhappy.\n"
    "And so the problem remained; lots of the people were mean, and most of them were miserable, even the ones with digital watches.\n"
    "Many were increasingly of the opinion that they’d all made a big mistake in coming down from the trees in the first place. And some said that even the trees had been a bad move, and that no one should ever have left the oceans.\n"
    "And then, one Thursday, nearly two thousand years after one man had been nailed to a tree for saying how great it would be to be nice to people for a change, one girl sitting on her own in a small cafe in Rickmansworth suddenly realized what it was that had been going wrong all this time, and she finally knew how the world could be made a good and happy place. This time it was right, it would work, and no one would have to get nailed to anything.\n"
    "Sadly, however, before she could get to a phone to tell anyone about it, a terribly stupid catastrophe occurred, and the idea was lost forever.\n"
    "This is not her story.\n"
    "But it is the story of that terrible stupid catastrophe and some of its consequences.\n"
    "It is also the story of a book, a book called The Hitchhiker’s Guide to the Galaxy – not an Earth book, never published on Earth, and until the terrible catastrophe occurred, never seen or heard of by any Earthman.\n"
    "Nevertheless, a wholly remarkable book.\n"
    "In fact it was probably the most remarkable book ever to come out of the great publishing houses of Ursa Minor – of which no Earthman had ever heard either.\n"
    "Not only is it a wholly remarkable book, it is also a highly successful one – more popular than the Celestial Home Care Omnibus, better selling than Fifty More Things to do in Zero Gravity, and more controversial than Oolon Colluphid’s trilogy of philosophical blockbusters Where God Went Wrong, Some More of God’s Greatest Mistakes and Who is this God Person Anyway?\n"
    "In many of the more relaxed civilizations on the Outer Eastern Rim of the Galaxy, the Hitchhiker’s Guide has already supplanted the great Encyclopedia Galactica as the standard repository of all knowledge and wisdom, for though it has many omissions and contains much that is apocryphal, or at least wildly inaccurate, it scores over the older, more pedestrian work in two important respects.\n"
    "First, it is slightly cheaper; and secondly it has the words Don’t Panic inscribed in large friendly letters on its cover.\n"
    "But the story of this terrible, stupid Thursday, the story of its extraordi- nary consequences, and the story of how these consequences are inextricably intertwined with this remarkable book begins very simply.\n"
    "It begins with a house.\n"
    "Far out in the uncharted backwaters of the unfashionable end of the western spiral arm of the Galaxy lies a small unregarded yellow sun. Orbiting this at a distance of roughly ninety-two million miles is an utterly insignificant little blue green planet whose ape-descended life forms are so amazingly primitive that they still think digital watches are a pretty neat idea.\n"
    "This planet has – or rather had – a problem, which was this: most of the people on it were unhappy for pretty much of the time. Many solutions were suggested for this problem, but most of these were largely concerned with the movements of small green pieces of paper, which is odd because on the whole it wasn’t the small green pieces of paper that were unhappy.\n"
    "And so the problem remained; lots of the people were mean, and most of them were miserable, even the ones with digital watches.\n"
    "Many were increasingly of the opinion that they’d all made a big mistake in coming down from the trees in the first place. And some said that even the trees had been a bad move, and that no one should ever have left the oceans.\n"
    "And then, one Thursday, nearly two thousand years after one man had been nailed to a tree for saying how great it would be to be nice to people for a change, one girl sitting on her own in a small cafe in Rickmansworth suddenly realized what it was that had been going wrong all this time, and she finally knew how the world could be made a good and happy place. This time it was right, it would work, and no one would have to get nailed to anything.\n"
    "Sadly, however, before she could get to a phone to tell anyone about it, a terribly stupid catastrophe occurred, and the idea was lost forever.\n"
    "This is not her story.\n"
    "But it is the story of that terrible stupid catastrophe and some of its consequences.\n"
    "It is also the story of a book, a book called The Hitchhiker’s Guide to the Galaxy – not an Earth book, never published on Earth, and until the terrible catastrophe occurred, never seen or heard of by any Earthman.\n"
    "Nevertheless, a wholly remarkable book.\n"
    "In fact it was probably the most remarkable book ever to come out of the great publishing houses of Ursa Minor – of which no Earthman had ever heard either.\n"
    "Not only is it a wholly remarkable book, it is also a highly successful one – more popular than the Celestial Home Care Omnibus, better selling than Fifty More Things to do in Zero Gravity, and more controversial than Oolon Colluphid’s trilogy of philosophical blockbusters Where God Went Wrong, Some More of God’s Greatest Mistakes and Who is this God Person Anyway?\n"
    "In many of the more relaxed civilizations on the Outer Eastern Rim of the Galaxy, the Hitchhiker’s Guide has already supplanted the great Encyclopedia Galactica as the standard repository of all knowledge and wisdom, for though it has many omissions and contains much that is apocryphal, or at least wildly inaccurate, it scores over the older, more pedestrian work in two important respects.\n"
    "First, it is slightly cheaper; and secondly it has the words Don’t Panic inscribed in large friendly letters on its cover.\n"
    "But the story of this terrible, stupid Thursday, the story of its extraordi- nary consequences, and the story of how these consequences are inextricably intertwined with this remarkable book begins very simply.\n"
    "It begins with a house.\n"
    "Far out in the uncharted backwaters of the unfashionable end of the western spiral arm of the Galaxy lies a small unregarded yellow sun. Orbiting this at a distance of roughly ninety-two million miles is an utterly insignificant little blue green planet whose ape-descended life forms are so amazingly primitive that they still think digital watches are a pretty neat idea.\n"
    "This planet has – or rather had – a problem, which was this: most of the people on it were unhappy for pretty much of the time. Many solutions were suggested for this problem, but most of these were largely concerned with the movements of small green pieces of paper, which is odd because on the whole it wasn’t the small green pieces of paper that were unhappy.\n"
    "And so the problem remained; lots of the people were mean, and most of them were miserable, even the ones with digital watches.\n"
    "Many were increasingly of the opinion that they’d all made a big mistake in coming down from the trees in the first place. And some said that even the trees had been a bad move, and that no one should ever have left the oceans.\n"
    "And then, one Thursday, nearly two thousand years after one man had been nailed to a tree for saying how great it would be to be nice to people for a change, one girl sitting on her own in a small cafe in Rickmansworth suddenly realized what it was that had been going wrong all this time, and she finally knew how the world could be made a good and happy place. This time it was right, it would work, and no one would have to get nailed to anything.\n"
    "Sadly, however, before she could get to a phone to tell anyone about it, a terribly stupid catastrophe occurred, and the idea was lost forever.\n"
    "This is not her story.\n"
    "But it is the story of that terrible stupid catastrophe and some of its consequences.\n"
    "It is also the story of a book, a book called The Hitchhiker’s Guide to the Galaxy – not an Earth book, never published on Earth, and until the terrible catastrophe occurred, never seen or heard of by any Earthman.\n"
    "Nevertheless, a wholly remarkable book.\n"
    "In fact it was probably the most remarkable book ever to come out of the great publishing houses of Ursa Minor – of which no Earthman had ever heard either.\n"
    "Not only is it a wholly remarkable book, it is also a highly successful one – more popular than the Celestial Home Care Omnibus, better selling than Fifty More Things to do in Zero Gravity, and more controversial than Oolon Colluphid’s trilogy of philosophical blockbusters Where God Went Wrong, Some More of God’s Greatest Mistakes and Who is this God Person Anyway?\n"
    "In many of the more relaxed civilizations on the Outer Eastern Rim of the Galaxy, the Hitchhiker’s Guide has already supplanted the great Encyclopedia Galactica as the standard repository of all knowledge and wisdom, for though it has many omissions and contains much that is apocryphal, or at least wildly inaccurate, it scores over the older, more pedestrian work in two important respects.\n"
    "First, it is slightly cheaper; and secondly it has the words Don’t Panic inscribed in large friendly letters on its cover.\n"
    "But the story of this terrible, stupid Thursday, the story of its extraordi- nary consequences, and the story of how these consequences are inextricably intertwined with this remarkable book begins very simply.\n"
    "It begins with a house.\n"
    "Far out in the uncharted backwaters of the unfashionable end of the western spiral arm of the Galaxy lies a small unregarded yellow sun. Orbiting this at a distance of roughly ninety-two million miles is an utterly insignificant little blue green planet whose ape-descended life forms are so amazingly primitive that they still think digital watches are a pretty neat idea.\n"
    "This planet has – or rather had – a problem, which was this: most of the people on it were unhappy for pretty much of the time. Many solutions were suggested for this problem, but most of these were largely concerned with the movements of small green pieces of paper, which is odd because on the whole it wasn’t the small green pieces of paper that were unhappy.\n"
    "And so the problem remained; lots of the people were mean, and most of them were miserable, even the ones with digital watches.\n"
    "Many were increasingly of the opinion that they’d all made a big mistake in coming down from the trees in the first place. And some said that even the trees had been a bad move, and that no one should ever have left the oceans.\n"
    "And then, one Thursday, nearly two thousand years after one man had been nailed to a tree for saying how great it would be to be nice to people for a change, one girl sitting on her own in a small cafe in Rickmansworth suddenly realized what it was that had been going wrong all this time, and she finally knew how the world could be made a good and happy place. This time it was right, it would work, and no one would have to get nailed to anything.\n"
    "Sadly, however, before she could get to a phone to tell anyone about it, a terribly stupid catastrophe occurred, and the idea was lost forever.\n"
    "This is not her story.\n"
    "But it is the story of that terrible stupid catastrophe and some of its consequences.\n"
    "It is also the story of a book, a book called The Hitchhiker’s Guide to the Galaxy – not an Earth book, never published on Earth, and until the terrible catastrophe occurred, never seen or heard of by any Earthman.\n"
    "Nevertheless, a wholly remarkable book.\n"
    "In fact it was probably the most remarkable book ever to come out of the great publishing houses of Ursa Minor – of which no Earthman had ever heard either.\n"
    "Not only is it a wholly remarkable book, it is also a highly successful one – more popular than the Celestial Home Care Omnibus, better selling than Fifty More Things to do in Zero Gravity, and more controversial than Oolon Colluphid’s trilogy of philosophical blockbusters Where God Went Wrong, Some More of God’s Greatest Mistakes and Who is this God Person Anyway?\n"
    "In many of the more relaxed civilizations on the Outer Eastern Rim of the Galaxy, the Hitchhiker’s Guide has already supplanted the great Encyclopedia Galactica as the standard repository of all knowledge and wisdom, for though it has many omissions and contains much that is apocryphal, or at least wildly inaccurate, it scores over the older, more pedestrian work in two important respects.\n"
    "First, it is slightly cheaper; and secondly it has the words Don’t Panic inscribed in large friendly letters on its cover.\n"
    "But the story of this terrible, stupid Thursday, the story of its extraordi- nary consequences, and the story of how these consequences are inextricably intertwined with this remarkable book begins very simply.\n"
    "It begins with a house.\n"
    "Far out in the uncharted backwaters of the unfashionable end of the western spiral arm of the Galaxy lies a small unregarded yellow sun. Orbiting this at a distance of roughly ninety-two million miles is an utterly insignificant little blue green planet whose ape-descended life forms are so amazingly primitive that they still think digital watches are a pretty neat idea.\n"
    "This planet has – or rather had – a problem, which was this: most of the people on it were unhappy for pretty much of the time. Many solutions were suggested for this problem, but most of these were largely concerned with the movements of small green pieces of paper, which is odd because on the whole it wasn’t the small green pieces of paper that were unhappy.\n"
    "And so the problem remained; lots of the people were mean, and most of them were miserable, even the ones with digital watches.\n"
    "Many were increasingly of the opinion that they’d all made a big mistake in coming down from the trees in the first place. And some said that even the trees had been a bad move, and that no one should ever have left the oceans.\n"
    "And then, one Thursday, nearly two thousand years after one man had been nailed to a tree for saying how great it would be to be nice to people for a change, one girl sitting on her own in a small cafe in Rickmansworth suddenly realized what it was that had been going wrong all this time, and she finally knew how the world could be made a good and happy place. This time it was right, it would work, and no one would have to get nailed to anything.\n"
    "Sadly, however, before she could get to a phone to tell anyone about it, a terribly stupid catastrophe occurred, and the idea was lost forever.\n"
    "This is not her story.\n"
    "But it is the story of that terrible stupid catastrophe and some of its consequences.\n"
    "It is also the story of a book, a book called The Hitchhiker’s Guide to the Galaxy – not an Earth book, never published on Earth, and until the terrible catastrophe occurred, never seen or heard of by any Earthman.\n"
    "Nevertheless, a wholly remarkable book.\n"
    "In fact it was probably the most remarkable book ever to come out of the great publishing houses of Ursa Minor – of which no Earthman had ever heard either.\n"
    "Not only is it a wholly remarkable book, it is also a highly successful one – more popular than the Celestial Home Care Omnibus, better selling than Fifty More Things to do in Zero Gravity, and more controversial than Oolon Colluphid’s trilogy of philosophical blockbusters Where God Went Wrong, Some More of God’s Greatest Mistakes and Who is this God Person Anyway?\n"
    "In many of the more relaxed civilizations on the Outer Eastern Rim of the Galaxy, the Hitchhiker’s Guide has already supplanted the great Encyclopedia Galactica as the standard repository of all knowledge and wisdom, for though it has many omissions and contains much that is apocryphal, or at least wildly inaccurate, it scores over the older, more pedestrian work in two important respects.\n"
    "First, it is slightly cheaper; and secondly it has the words Don’t Panic inscribed in large friendly letters on its cover.\n"
    "But the story of this terrible, stupid Thursday, the story of its extraordi- nary consequences, and the story of how these consequences are inextricably intertwined with this remarkable book begins very simply.\n"
    "It begins with a house.\n"
    "Far out in the uncharted backwaters of the unfashionable end of the western spiral arm of the Galaxy lies a small unregarded yellow sun. Orbiting this at a distance of roughly ninety-two million miles is an utterly insignificant little blue green planet whose ape-descended life forms are so amazingly primitive that they still think digital watches are a pretty neat idea.\n"
    "This planet has – or rather had – a problem, which was this: most of the people on it were unhappy for pretty much of the time. Many solutions were suggested for this problem, but most of these were largely concerned with the movements of small green pieces of paper, which is odd because on the whole it wasn’t the small green pieces of paper that were unhappy.\n"
    "And so the problem remained; lots of the people were mean, and most of them were miserable, even the ones with digital watches.\n"
    "Many were increasingly of the opinion that they’d all made a big mistake in coming down from the trees in the first place. And some said that even the trees had been a bad move, and that no one should ever have left the oceans.\n"
    "And then, one Thursday, nearly two thousand years after one man had been nailed to a tree for saying how great it would be to be nice to people for a change, one girl sitting on her own in a small cafe in Rickmansworth suddenly realized what it was that had been going wrong all this time, and she finally knew how the world could be made a good and happy place. This time it was right, it would work, and no one would have to get nailed to anything.\n"
    "Sadly, however, before she could get to a phone to tell anyone about it, a terribly stupid catastrophe occurred, and the idea was lost forever.\n"
    "This is not her story.\n"
    "But it is the story of that terrible stupid catastrophe and some of its consequences.\n"
    "It is also the story of a book, a book called The Hitchhiker’s Guide to the Galaxy – not an Earth book, never published on Earth, and until the terrible catastrophe occurred, never seen or heard of by any Earthman.\n"
    "Nevertheless, a wholly remarkable book.\n"
    "In fact it was probably the most remarkable book ever to come out of the great publishing houses of Ursa Minor – of which no Earthman had ever heard either.\n"
    "Not only is it a wholly remarkable book, it is also a highly successful one – more popular than the Celestial Home Care Omnibus, better selling than Fifty More Things to do in Zero Gravity, and more controversial than Oolon Colluphid’s trilogy of philosophical blockbusters Where God Went Wrong, Some More of God’s Greatest Mistakes and Who is this God Person Anyway?\n"
    "In many of the more relaxed civilizations on the Outer Eastern Rim of the Galaxy, the Hitchhiker’s Guide has already supplanted the great Encyclopedia Galactica as the standard repository of all knowledge and wisdom, for though it has many omissions and contains much that is apocryphal, or at least wildly inaccurate, it scores over the older, more pedestrian work in two important respects.\n"
    "First, it is slightly cheaper; and secondly it has the words Don’t Panic inscribed in large friendly letters on its cover.\n"
    "But the story of this terrible, stupid Thursday, the story of its extraordi- nary consequences, and the story of how these consequences are inextricably intertwined with this remarkable book begins very simply.\n"
    "It begins with a house.\n"
    
};

static uint8_t dataBuffer2[64*1024] = {
    "But it is the story of that terrible stupid catastrophe and some of its consequences.\n"
    "It is also the story of a book, a book called The Hitchhiker’s Guide to the Galaxy – not an Earth book, never published on Earth, and until the terrible catastrophe occurred, never seen or heard of by any Earthman.\n"
    "Nevertheless, a wholly remarkable book.\n"
    "In fact it was probably the most remarkable book ever to come out of the great publishing houses of Ursa Minor – of which no Earthman had ever heard either.\n"
    "Not only is it a wholly remarkable book, it is also a highly successful one – more popular than the Celestial Home Care Omnibus, better selling than Fifty More Things to do in Zero Gravity, and more controversial than Oolon Colluphid’s trilogy of philosophical blockbusters Where God Went Wrong, Some More of God’s Greatest Mistakes and Who is this God Person Anyway?\n"
    "In many of the more relaxed civilizations on the Outer Eastern Rim of the Galaxy, the Hitchhiker’s Guide has already supplanted the great Encyclopedia Galactica as the standard repository of all knowledge and wisdom, for though it has many omissions and contains much that is apocryphal, or at least wildly inaccurate, it scores over the older, more pedestrian work in two important respects.\n"
    "First, it is slightly cheaper; and secondly it has the words Don’t Panic inscribed in large friendly letters on its cover.\n"
    "But the story of this terrible, stupid Thursday, the story of its extraordi- nary consequences, and the story of how these consequences are inextricably intertwined with this remarkable book begins very simply.\n"
    "It begins with a house.\n"
};


/**********************************************************************************************************************/
/*  Macro to initialize the library and open two sessions                                                             */
/**********************************************************************************************************************/
#define PREPARE(a,b)                                                            \
    gFailed = 0;                                                                \
    TRDP_ERR_T      err         = TRDP_NO_ERR;                                  \
    TRDP_APP_SESSION_T   appHandle1  = NULL, appHandle2 = NULL;                      \
    {                                                                           \
                                                                                \
        fprintf(gFp, "\n---- Start of %s (%s) ---------\n\n", __FUNCTION__, a); \
        appHandle1 = test_init(dbgOut, &gSession1, (b));                      \
        if (appHandle1 == NULL)                                                 \
        {                                                                       \
            gFailed = 1;                                                         \
            goto end;                                                           \
        }                                                                       \
        appHandle2 = test_init(NULL, &gSession2, (b));                        \
        if (appHandle2 == NULL)                                                 \
        {                                                                       \
            gFailed = 1;                                                         \
            goto end;                                                           \
        }                                                                       \
    }

/**********************************************************************************************************************/
/*  Macro to initialize the library and open one session                                                              */
/**********************************************************************************************************************/
#define PREPARE1(a)                                                             \
    gFailed = 0;                                                             \
    TRDP_ERR_T      err         = TRDP_NO_ERR;                                  \
    TRDP_APP_SESSION_T   appHandle1  = NULL, appHandle2 = NULL;                      \
    {                                                                           \
                                                                                \
        fprintf(gFp, "\n---- Start of %s (%s) ---------\n\n", __FUNCTION__, a); \
        appHandle1 = test_init(dbgOut, &gSession1, "");                      \
        if (appHandle1 == NULL)                                                 \
        {                                                                       \
            gFailed = 1;                                                         \
            goto end;                                                           \
        }                                                                       \
    }


/**********************************************************************************************************************/
/*  Macro to initialize common function testing                                                                       */
/**********************************************************************************************************************/
#define PREPARE_COM(a)                                                    \
    gFailed = 0;                                                       \
    TRDP_ERR_T err = TRDP_NO_ERR;                                         \
    {                                                                     \
                                                                          \
        printf("\n---- Start of %s (%s) ---------\n\n", __FUNCTION__, a); \
                                                                          \
    }



/**********************************************************************************************************************/
/*  Macro to terminate the library and close two sessions                                                             */
/**********************************************************************************************************************/
#define CLEANUP                                                               \
end:                                                                          \
    {                                                                         \
        test_deinit(&gSession1 ,&gSession2);                \
                                                                              \
        if (gFailed) {                                                         \
            fprintf(gFp, "\n###########  FAILED!  ###############\nlasterr = %d", err); }      \
        else{                                                                 \
            fprintf(gFp, "\n-----------  Success  ---------------\n"); }      \
                                                                              \
        fprintf(gFp, "--------- End of %s --------------\n\n", __FUNCTION__); \
                                                                              \
        return gFailed;                                                        \
    }

/**********************************************************************************************************************/
/*  Macro to check for error and terminate the test                                                                   */
/**********************************************************************************************************************/
#define IF_ERROR(message)                                   \
    if (err != TRDP_NO_ERR)                                 \
    {                                                       \
        fprintf(gFp, "### %s (error: %d)\n", message, err); \
        gFailed = 1;                                         \
        goto end;                                           \
    }

/**********************************************************************************************************************/
/*  Macro to terminate the test                                                                                       */
/**********************************************************************************************************************/
#define FAILED(message)                \
    fprintf(gFp, "### %s\n", message); \
    gFailed = 1;                        \
    goto end;                          \

/**********************************************************************************************************************/
/** callback routine for TRDP logging/error output
 *
 *  @param[in]      pRefCon            user supplied context pointer
 *  @param[in]        category        Log category (Error, Warning, Info etc.)
 *  @param[in]        pTime            pointer to NULL-terminated string of time stamp
 *  @param[in]        pFile            pointer to NULL-terminated string of source module
 *  @param[in]        lineNumber        line
 *  @param[in]        pMsgStr         pointer to NULL-terminated string
 *  @retval         none
 */
static void dbgOut (
    void        *pRefCon,
    TRDP_LOG_T  category,
    const CHAR8 *pTime,
    const CHAR8 *pFile,
    UINT16      lineNumber,
    const CHAR8 *pMsgStr)
{
    const char *catStr[] = {"**Error:", "Warning:", "   Info:", "  Debug:"};


    if (category != VOS_LOG_DBG && category != VOS_LOG_INFO)
    {
        fprintf(gFp, "%s %s %s:%d %s",
                strrchr(pTime, '-') + 1,
                catStr[category],
                strrchr(pFile, '/') + 1,
                lineNumber,
                pMsgStr);
    }
    //else if (strstr(pMsgStr, "vos_mem") != NULL)
    {
        //fprintf(gFp, "### %s", pMsgStr);
    }
}

/**********************************************************************************************************************/
/** trdp processing loop (thread)
 *
 *  @param[in]      pRefCon            user supplied context pointer
 *  @param[in]        category        Log category (Error, Warning, Info etc.)
 *  @param[in]        pTime            pointer to NULL-terminated string of time stamp
 *  @param[in]        pFile            pointer to NULL-terminated string of source module
 *  @param[in]        lineNumber        line
 *  @param[in]        pMsgStr         pointer to NULL-terminated string
 *  @retval         none
 */
static void trdp_loop (void* pArg)
{
    TRDP_THREAD_SESSION_T  *pSession = (TRDP_THREAD_SESSION_T*) pArg;
    /*
        Enter the main processing loop.
     */
    while (pSession->threadId)
    {
        TRDP_FDS_T  rfds;
        INT32       noDesc;
        INT32       rv;
        TRDP_TIME_T  tv;
        TRDP_TIME_T  max_tv = {0u, 100000};
        TRDP_TIME_T  min_tv = {0u, 10000};
        
        /*
         Prepare the file descriptor set for the select call.
         Additional descriptors can be added here.
         */
        FD_ZERO(&rfds);
        
        /*
         Compute the min. timeout value for select.
         This way we can guarantee that PDs are sent in time
         with minimum CPU load and minimum jitter.
         */
        (void)tlc_getInterval(pSession->appHandle, &tv, (TRDP_FDS_T *) &rfds, &noDesc);
        
        /*
         The wait time for select must consider cycle times and timeouts of
         the PD packets received or sent.
         If we need to poll something faster than the lowest PD cycle,
         we need to set the maximum time out our self.
         */
        if (vos_cmpTime(&tv, &max_tv) > 0)
        {
            tv = max_tv;
        }
        
        if (vos_cmpTime(&tv, &min_tv) < 0)
        {
            tv = min_tv;
        }
        /*
         Select() will wait for ready descriptors or time out,
         what ever comes first.
         */
        rv = vos_select(noDesc + 1, &rfds, NULL, NULL, &tv);
        
        /*
         Check for overdue PDs (sending and receiving)
         Send any pending PDs if it's time...
         Detect missing PDs...
         'rv' will be updated to show the handled events, if there are
         more than one...
         The callback function will be called from within the tlc_process
         function (in it's context and thread)!
         */
        (void) tlc_process(pSession->appHandle, &rfds, &rv);
        
    }

    /*
     *    We always clean up behind us!
     */

    (void)tlc_closeSession(pSession->appHandle);
    pSession->appHandle = NULL;
}

/**********************************************************************************************************************/
/* Print a sensible usage message */
static void usage (const char *appName)
{
    printf("Usage of %s\n", appName);
    printf("Run defined test suite on a single machine using two application sessions.\n"
           "Pre-condition: There must be two IP addresses/interfaces configured and connected by a switch.\n"
           "Arguments are:\n"
           "-o <own IP address> (default 10.0.1.100)\n"
           "-i <second IP address> (default 10.0.1.101)\n"
           "-t <destination MC> (default 239.0.1.1)\n"
           "-m number of test to run (1...n, default 0 = run all tests)\n"
           "-v print version and quit\n"
           "-h this list\n"
           );
}

/**********************************************************************************************************************/
/** Find 2 interface IPs in the range 10.0.x.y
 *  to be used for ping-pong testing
 *  @param
 */
static void iface_init (
    TRDP_IP_ADDR_T          *pIP1,
    TRDP_IP_ADDR_T          *pIP2)
{
    if ((*pIP1 == 0) && (*pIP2 == 0))
    {
        /* find suitable addresses if not set */
        
    }
}

/**********************************************************************************************************************/
/** common initialisation
 *
 *  @retval         application session handle
 */
static TRDP_APP_SESSION_T test_init (
    TRDP_PRINT_DBG_T        dbgout,
    TRDP_THREAD_SESSION_T   *pSession,
    const char              *name)
{
    TRDP_ERR_T          err     = TRDP_NO_ERR;
    pSession->appHandle = NULL;
    
    if (dbgout != NULL)
    {
        /* for debugging & testing we use dynamic memory allocation (heap) */
        err = tlc_init(dbgout, NULL, NULL);
    }
    if (err == TRDP_NO_ERR)                 /* We ignore double init here */
    {
        tlc_openSession(&pSession->appHandle, pSession->ifaceIP, 0u, NULL, NULL, NULL, NULL);
        /* On error the handle will be NULL... */
    }
    
    if (err == TRDP_NO_ERR)
    {
        (void) vos_threadCreate(&pSession->threadId, name, VOS_THREAD_POLICY_OTHER, 0u, 0u, 0u,
                               trdp_loop, pSession);
    }
    return pSession->appHandle;
}

/**********************************************************************************************************************/
/** common initialisation
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */
static void test_deinit (
    TRDP_THREAD_SESSION_T   *pSession1,
    TRDP_THREAD_SESSION_T   *pSession2)
{
    if (pSession1)
    {
        pSession1->threadRun = 0;
        vos_threadTerminate(pSession1->threadId);
        vos_threadDelay(100000);
    }
    if (pSession2)
    {
        pSession2->threadRun = 0;
        vos_threadTerminate(pSession2->threadId);
        vos_threadDelay(100000);
    }
    tlc_terminate();
}

/**********************************************************************************************************************/
/**********************************************************************************************************************/
/**********************************************************************************************************************/
/******************************************** Testing starts here *****************************************************/
/**********************************************************************************************************************/
/**********************************************************************************************************************/
/**********************************************************************************************************************/
/**********************************************************************************************************************/





/**********************************************************************************************************************/
/** PD publish and subscribe
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */
static int test1 (int argc, char *argv[])
{
    PREPARE("Basic PD publish and subscribe, polling (#128 ComId = 0)", "test"); /* allocates appHandle1, appHandle2, failed = 0, err */

    /* ------------------------- test code starts here --------------------------- */

    {
        TRDP_PUB_T      pubHandle;
        TRDP_SUB_T      subHandle;

#define TEST1_COMID     0u
#define TEST1_INTERVAL  100000u
#define TEST1_DATA      "Hello World!"
#define TEST1_DATA_LEN  16u

        /*    Copy the packet into the internal send queue, prepare for sending.    */

        err = tlp_publish(gSession1.appHandle, &pubHandle, TEST1_COMID, 0u, 0u,
                          0u, //gSession1.ifaceIP,                   /* Source */
                          gSession2.ifaceIP,//gDestMC,               /* Destination */
                          TEST1_INTERVAL,
                          0u, TRDP_FLAGS_DEFAULT, NULL, (UINT8*) TEST1_DATA, TEST1_DATA_LEN);
        
        IF_ERROR("tlp_publish");

        err = tlp_subscribe(gSession2.appHandle, &subHandle, NULL, NULL,
                            TEST1_COMID, 0u, 0u,
                            0u,//gSession1.ifaceIP,                  /* Source */
                            0u,//gDestMC,                            /* Destination */
                            TRDP_FLAGS_DEFAULT, TEST1_INTERVAL * 3, TRDP_TO_DEFAULT);
        

        IF_ERROR("tlp_subscribe");

        /*
         Enter the main processing loop.
         */
        int counter = 0;
        while (counter < 50)         /* 5 seconds */
        {
            char            data1[1432u];
            char            data2[1432u];
            UINT32          dataSize2 = sizeof(data2);
            TRDP_PD_INFO_T  pdInfo;

            sprintf(data1, "Just a Counter: %08d", counter++);

            err = tlp_put(gSession1.appHandle, pubHandle, (UINT8 *) data1, (UINT32) strlen(data1));
            IF_ERROR("tap_put");

            usleep(100000u);

            err = tlp_get(gSession2.appHandle, subHandle, &pdInfo, (UINT8 *) data2, &dataSize2);
            if (err == TRDP_NODATA_ERR)
            {
                continue;
            }

            if (err != TRDP_NO_ERR)
            {
                fprintf(gFp, "### tlp_get error: %d\n", err);
                
                gFailed = 1;
                goto end;
                
            }
            else
            {
                if (memcmp(data1, data2, dataSize2) == 0)
                {
                    fprintf(gFp, "received data matches (seq: %u, size: %u)\n", pdInfo.seqCount, dataSize2);
                }
            }
        }
    }


    /* ------------------------- test code ends here --------------------------- */

    CLEANUP;
}

/**********************************************************************************************************************/
/** test2
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */

static void test2PDcallBack (
                   void                    *pRefCon,
                   TRDP_APP_SESSION_T      appHandle,
                   const TRDP_PD_INFO_T    *pMsg,
                   UINT8                   *pData,
                   UINT32                  dataSize)
{
    void * pSentdata = (void*) pMsg->pUserRef;

    /*    Check why we have been called    */
    switch (pMsg->resultCode)
    {
        case TRDP_NO_ERR:
            if ((pSentdata != NULL) && (pData != NULL) && (memcmp(pData, pSentdata, dataSize) == 0))
            {
                fprintf(gFp, "received data matches (seq: %u, size: %u)\n", pMsg->seqCount, dataSize);
            }
            break;
            
        case TRDP_TIMEOUT_ERR:
            /* The application can decide here if old data shall be invalidated or kept    */
            fprintf(gFp, "Packet timed out (ComId %d, SrcIP: %s)\n",
                   pMsg->comId,
                   vos_ipDotted(pMsg->srcIpAddr));
            break;
        default:
            fprintf(gFp, "Error on packet received (ComId %d), err = %d\n",
                   pMsg->comId,
                   pMsg->resultCode);
            break;
    }
}

static int test2 (int argc, char *argv[])
{
    PREPARE("Publish & Subscribe, Callback", "test"); /* allocates appHandle1, appHandle2, failed = 0, err */

    /* ------------------------- test code starts here --------------------------- */

    {
        TRDP_PUB_T      pubHandle;
        TRDP_SUB_T      subHandle;
        char            data1[1432u];
        
#define TEST2_COMID     1000u
#define TEST2_INTERVAL  100000u
#define TEST2_DATA      "Hello World!"
        
        /*    Copy the packet into the internal send queue, prepare for sending.    */
        
        err = tlp_publish(gSession1.appHandle, &pubHandle, TEST2_COMID, 0u, 0u,
                          0u, //gSession1.ifaceIP,                   /* Source */
                          gSession2.ifaceIP,//gDestMC,               /* Destination */
                          TEST2_INTERVAL,
                          0u, TRDP_FLAGS_DEFAULT, NULL, NULL, 0u);
        
        IF_ERROR("tlp_publish");
        
        err = tlp_subscribe(gSession2.appHandle, &subHandle, data1, test2PDcallBack,
                            TEST2_COMID, 0u, 0u,
                            0u,//gSession1.ifaceIP,                  /* Source */
                            0u,//gDestMC,                            /* Destination */
                            TRDP_FLAGS_CALLBACK, TEST2_INTERVAL * 3, TRDP_TO_DEFAULT);
        
        
        IF_ERROR("tlp_subscribe");
        
        /*
         Enter the main processing loop.
         */
        int counter = 0;
        while (counter < 5)         /* 0.5 seconds */
        {
            
            sprintf(data1, "Just a Counter: %08d", counter++);
            
            err = tlp_put(gSession1.appHandle, pubHandle, (UINT8 *) data1, (UINT32) strlen(data1));
            IF_ERROR("tap_put");
            
            usleep(100000u);
            
        }
    }

    /* ------------------------- test code ends here --------------------------- */


    CLEANUP;
}

/**********************************************************************************************************************/
/** test3 tlp_get
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */
static int test3 (int argc, char *argv[])
{
    PREPARE("Ticket #140: tlp_get reports immediately TRDP_TIMEOUT_ERR", "test"); /* allocates appHandle1, appHandle2, failed = 0, err */

    /* ------------------------- test code starts here --------------------------- */

    {
        TRDP_SUB_T      subHandle;
        
#define TEST3_COMID     1000u
#define TEST3_INTERVAL  100000u
        
        
        err = tlp_subscribe(gSession2.appHandle, &subHandle, NULL, NULL,
                            TEST3_COMID, 0u, 0u,
                            0u,//gSession1.ifaceIP,                  /* Source */
                            0u,//gDestMC,                            /* Destination */
                            TRDP_FLAGS_DEFAULT, TRDP_TIMER_FOREVER, TRDP_TO_DEFAULT);
        
        
        IF_ERROR("tlp_subscribe");
        
        /*
         Enter the main processing loop.
         */
        int counter = 0;
        while (counter++ < 50)         /* 5 seconds */
        {
            char            data2[1432u];
            UINT32          dataSize2 = sizeof(data2);
            TRDP_PD_INFO_T  pdInfo;
            
            usleep(TEST3_INTERVAL);
            
            err = tlp_get(gSession2.appHandle, subHandle, &pdInfo, (UINT8 *) data2, &dataSize2);
            if (err == TRDP_NODATA_ERR)
            {
                fprintf(gFp, ".");
                fflush(gFp);
                continue;
            }
            
            if (err != TRDP_NO_ERR)
            {
                fprintf(gFp, "\n### tlp_get error: %d\n", err);
                
                gFailed = 1;
                goto end;
                
            }

        }
        fprintf(gFp, "\n");
    }

    /* ------------------------- test code ends here --------------------------- */


    CLEANUP;
}

/**********************************************************************************************************************/
/** test4 PD PULL Request
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */
static int test4 (int argc, char *argv[])
{
    PREPARE("#153 (two PDs on one pull request", "test"); /* allocates appHandle1, appHandle2, failed = 0, err */

    /* ------------------------- test code starts here --------------------------- */

    {
        {
            TRDP_PUB_T      pubHandle;
            TRDP_SUB_T      subHandle;
            
#define TEST4_COMID     1000u
#define TEST4_INTERVAL  100000u
#define TEST4_DATA      "Hello World!"
#define TEST4_DATA_LEN  16u
            
            /*    Session1 Install subscriber and publisher for PULL (interval = 0).    */
            
            err = tlp_subscribe(gSession1.appHandle, &subHandle, NULL, NULL,
                                TEST4_COMID, 0u, 0u,
                                0u,
                                gDestMC,                            /* MC group */
                                TRDP_FLAGS_NONE, 0, TRDP_TO_DEFAULT);
            
            
            IF_ERROR("tlp_subscribe");

            err = tlp_publish(gSession1.appHandle, &pubHandle, TEST4_COMID, 0u, 0u,
                              0u,
                              gDestMC,                              /* Destination */
                              0u,
                              0u, TRDP_FLAGS_DEFAULT, NULL, (UINT8*) TEST4_DATA, TEST4_DATA_LEN);
            
            IF_ERROR("tlp_publish");

            /*    Session2 Install subscriber and do a request for PULL.    */
            
            err = tlp_subscribe(gSession2.appHandle, &subHandle, NULL, NULL,
                                TEST4_COMID, 0u, 0u,
                                0u,                                 /* Source */
                                gDestMC,                            /* Destination */
                                TRDP_FLAGS_DEFAULT, TEST4_INTERVAL * 3, TRDP_TO_DEFAULT);
            
            
            IF_ERROR("tlp_subscribe");
            
            err = tlp_request(gSession2.appHandle, subHandle,
                              TEST4_COMID, 0u, 0u, gSession2.ifaceIP, gSession1.ifaceIP,
                              0u, TRDP_FLAGS_NONE, NULL , NULL , 0u, TEST4_COMID, gDestMC);

            IF_ERROR("tlp_request");

            /*
             Enter the main processing loop.
             */
            int counter = 0;
            while (counter++ < 50)         /* 5 seconds */
            {
                char            data2[1432u];
                UINT32          dataSize2 = sizeof(data2);
                TRDP_PD_INFO_T  pdInfo;
                
                usleep(100000u);
                
                err = tlp_get(gSession2.appHandle, subHandle, &pdInfo, (UINT8 *) data2, &dataSize2);
                if (err == TRDP_NODATA_ERR)
                {
                    continue;
                }
                
                if (err == TRDP_TIMEOUT_ERR)
                {
                    continue;
                }

                if (err != TRDP_NO_ERR)
                {
                    fprintf(gFp, "### tlp_get error: %d\n", err);
                    
                    gFailed = 1;
                    goto end;
                    
                }
                else
                {
                    fprintf(gFp, "received data from pull: %s (seq: %u, size: %u)\n", data2, pdInfo.seqCount, dataSize2);
                    gFailed = 0;
                    goto end;
                }
            }
        }
    }

    /* ------------------------- test code ends here --------------------------- */


    CLEANUP;
}

/**********************************************************************************************************************/
/** test5 MD Request - Reply - Confirm
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */

#define                 TEST5_STRING_COMID       1000u
#define                 TEST5_STRING_REQUEST     &dataBuffer1 //"Requesting data"
#define                 TEST5_STRING_REPLY       &dataBuffer2 //"Data transmission succeded"
        
static void  test5CBFunction (
    void                    *pRefCon,
    TRDP_APP_SESSION_T      appHandle,
    const TRDP_MD_INFO_T    *pMsg,
    UINT8                   *pData,
    UINT32                  dataSize)
{
    TRDP_ERR_T      err;
    TRDP_URI_USER_T srcURI =  "12345678901234567890123456789012";   // 32 chars
    
    if (pMsg->resultCode == TRDP_REPLYTO_ERR)
    {
        fprintf(gFp, "->> Reply timed out (ComId %u)\n", pMsg->comId);
        gFailed = 1;
    }
    else if ((pMsg->msgType == TRDP_MSG_MR) &&
        (pMsg->comId == TEST5_STRING_COMID))
    {
        if (pMsg->resultCode == TRDP_TIMEOUT_ERR)
        {
            fprintf(gFp, "->> Request timed out (ComId %u)\n", pMsg->comId);
            gFailed = 1;
        }
        else
        {
            //fprintf(gFp, "->> Request received (%s)\n", pData);
            if (memcmp(srcURI, pMsg->srcUserURI, 32u) != 0)
            {
                gFailed = 1;
                fprintf(gFp, "## srcUserURI wrong\n");
            }
            fprintf(gFp, "->> Sending reply\n");
            err = tlm_replyQuery(appHandle, &pMsg->sessionId, TEST5_STRING_COMID, 0u, 500000u, NULL,
                             (UINT8*)TEST5_STRING_REPLY, 63*1024/*strlen(TEST5_STRING_REPLY)*/);

            IF_ERROR("tlm_reply");
        }
    }
    
    else if ((pMsg->msgType == TRDP_MSG_MQ) &&
             (pMsg->comId == TEST5_STRING_COMID))
    {
        //fprintf(gFp, "->> Reply received (%s)\n", pData);
        fprintf(gFp, "->> Sending confirmation\n");
        err = tlm_confirm(appHandle, &pMsg->sessionId, 0u, NULL);
        
        IF_ERROR("tlm_confirm");
    }
    
    
    else if (pMsg->msgType == TRDP_MSG_MC)
    {
        fprintf(gFp, "->> Confirmation received (status = %u)\n", pMsg->userStatus);
    }
    else if ((pMsg->msgType == TRDP_MSG_MN) &&
             (pMsg->comId == TEST5_STRING_COMID))
    {
        if (strlen((char*)pMsg->sessionId) > 0)
        {
            gFailed = 1;
            fprintf(gFp, "#### ->> Notification received, sessionID = %16s\n", pMsg->sessionId);
        }
        else
        {
            gFailed = 0;
            fprintf(gFp, "->> Notification received, sessionID == 0\n");
        }
    }
    else
    {
        fprintf(gFp, "->> Unsolicited Message received (type = %0xhx)\n", pMsg->msgType);
        gFailed = 1;
    }
end:
    return;
}

static int test5 (int argc, char *argv[])
{
    PREPARE("TCP MD Request - Reply - Confirm, #149, #160", "test"); /* allocates appHandle1, appHandle2, failed = 0, err */

    /* ------------------------- test code starts here --------------------------- */

    {
        TRDP_UUID_T         sessionId1;
        TRDP_LIS_T          listenHandle;
        TRDP_URI_USER_T     destURI1 = "12345678901234567890123456789012";   // 32 chars
        TRDP_URI_USER_T     destURI2 = "12345678901234567890123456789012";   // 32 chars
        TRDP_URI_USER_T     srcURI   = "12345678901234567890123456789012";   // 32 chars

        err = tlm_addListener(appHandle2, &listenHandle, NULL, test5CBFunction,
                              TEST5_STRING_COMID, 0u, 0u, 0u, TRDP_FLAGS_CALLBACK | TRDP_FLAGS_TCP, destURI1);
        IF_ERROR("tlm_addListener1");
        fprintf(gFp, "->> MD TCP Listener1 set up\n");
        
        err = tlm_request(appHandle1, NULL, test5CBFunction, &sessionId1,
                          TEST5_STRING_COMID, 0u, 0u,
                          0u, gSession2.ifaceIP,
                          TRDP_FLAGS_CALLBACK | TRDP_FLAGS_TCP, 1u, 1000000u, 1u, NULL,
                          (UINT8*)TEST5_STRING_REQUEST, 63*1024/*strlen(TEST5_STRING_REQUEST)*/,
                          srcURI, destURI2);
        
        IF_ERROR("tlm_request1");
        fprintf(gFp, "->> MD TCP Request1 sent\n");
        
        vos_threadDelay(2000000u);
        
        
        err = tlm_request(appHandle1, NULL, test5CBFunction, &sessionId1,
                          TEST5_STRING_COMID, 0u, 0u,
                          0u, gSession2.ifaceIP,
                          TRDP_FLAGS_CALLBACK | TRDP_FLAGS_TCP, 1u, 1000000u, 1u, NULL,
                          (UINT8*)TEST5_STRING_REQUEST, 63*1024/*strlen(TEST5_STRING_REQUEST)*/,
                          srcURI, destURI2);
        
        IF_ERROR("tlm_request2");
        fprintf(gFp, "->> MD TCP Request2 sent\n");
        
        vos_threadDelay(2000000u);
        
        err = tlm_delListener (appHandle2, listenHandle);
        IF_ERROR("tlm_delListener2");
    }
    
   /* ------------------------- test code ends here --------------------------- */


    CLEANUP;
}

/**********************************************************************************************************************/
/** test6 (extension of test5, should fail)
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */
static int test6 (int argc, char *argv[])
{
    PREPARE("UDP MD Request - Reply - Confirm, #149", "test"); /* allocates appHandle1, appHandle2, failed = 0, err = TRDP_NO_ERR */

    /* ------------------------- test code starts here --------------------------- */

    {
        TRDP_UUID_T         sessionId1;
        TRDP_LIS_T          listenHandle;
        TRDP_URI_USER_T     destURI1 = "12345678901234567890123456789012";   // 32 chars
        TRDP_URI_USER_T     destURI2 = "1234567890123456789012345678901";   // 32 chars
        TRDP_URI_USER_T     srcURI   = "12345678901234567890123456789012";   // 32 chars
        
        err = tlm_addListener(appHandle2, &listenHandle, NULL, test5CBFunction,
                              TEST5_STRING_COMID, 0u, 0u, 0u, TRDP_FLAGS_CALLBACK, destURI1);
        IF_ERROR("tlm_addListener");
        fprintf(gFp, "->> MD Listener set up\n");
        
        err = tlm_request(appHandle1, NULL, test5CBFunction, &sessionId1,
                          TEST5_STRING_COMID, 0u, 0u,
                          0u, gSession2.ifaceIP,
                          TRDP_FLAGS_CALLBACK, 1u, 1000000u, 1u, NULL,
                          (UINT8*)TEST5_STRING_REQUEST, strlen(TEST5_STRING_REQUEST),
                          srcURI, destURI2);
        
        IF_ERROR("tlm_request");
        fprintf(gFp, "->> MD Request sent\n");
        
        vos_threadDelay(5000000u);

        /* Test expects to fail because of wrong destURI2, must timeout... */
        gFailed ^= gFailed;
        
        err = tlm_delListener (appHandle2, listenHandle);
        IF_ERROR("tlm_delListener");
    }

    /* ------------------------- test code ends here --------------------------- */


    CLEANUP;
}

/**********************************************************************************************************************/
/** test7
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */
static int test7 (int argc, char *argv[])
{
    PREPARE("UDP MD Notify no sessionID #127", "test"); /* allocates appHandle1, appHandle2, failed = 0, err = TRDP_NO_ERR */

    /* ------------------------- test code starts here --------------------------- */

    {
        TRDP_LIS_T          listenHandle;
        
        err = tlm_addListener(appHandle2, &listenHandle, NULL, test5CBFunction,
                              TEST5_STRING_COMID, 0u, 0u, 0u, TRDP_FLAGS_CALLBACK, NULL);
        IF_ERROR("tlm_addListener");
        fprintf(gFp, "->> MD Listener set up\n");
        
        err = tlm_notify(appHandle1, NULL, test5CBFunction, TEST5_STRING_COMID, 0u, 0u, 0u,
                         gSession2.ifaceIP, TRDP_FLAGS_CALLBACK, NULL,
                         (UINT8*)TEST5_STRING_REQUEST, strlen(TEST5_STRING_REQUEST), NULL, NULL );
        
        
        IF_ERROR("tlm_notify");
        fprintf(gFp, "->> MD Request sent\n");
        
        vos_threadDelay(5000000u);
        
        /* Test expects to fail because of wrong destURI2, must timeout... */
        gFailed ^= gFailed;
        
        err = tlm_delListener (appHandle2, listenHandle);
        IF_ERROR("tlm_delListener");
    }

    /* ------------------------- test code ends here --------------------------- */


    CLEANUP;
}

/**********************************************************************************************************************/
/** test8
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */
static int test8 (int argc, char *argv[])
{
    PREPARE("#153 (two PDs on one pull request? Receiver only", "test"); /* allocates appHandle1, appHandle2, failed = 0, err */

    /* ------------------------- test code starts here --------------------------- */

    {
        TRDP_PUB_T      pubHandle;
        TRDP_SUB_T      subHandle;
        
#define TEST8_COMID     1000u
#define TEST8_INTERVAL  100000u
#define TEST8_DATA      "Hello World!"
#define TEST8_DATA_LEN  16u
        
        /*    Session1 Install subscriber and publisher for PULL (interval = 0).    */
        
        err = tlp_subscribe(gSession1.appHandle, &subHandle, NULL, NULL,
                            TEST8_COMID, 0u, 0u,
                            0u,
                            gDestMC,                            /* MC group */
                            TRDP_FLAGS_NONE, 0, TRDP_TO_DEFAULT);
        
        
        IF_ERROR("tlp_subscribe");
        
        err = tlp_publish(gSession1.appHandle, &pubHandle, TEST8_COMID, 0u, 0u,
                          0u,
                          gDestMC,                              /* Destination */
                          0u,
                          0u, TRDP_FLAGS_DEFAULT, NULL, (UINT8*) TEST8_DATA, TEST8_DATA_LEN);
        
        IF_ERROR("tlp_publish");
        
        
        /*
         Enter the main processing loop.
         */
        int counter = 0;
        while (counter++ < 600)         /* 60 seconds */
        {
            char            data2[1432u];
            UINT32          dataSize2 = sizeof(data2);
            TRDP_PD_INFO_T  pdInfo;
            
            usleep(100000u);

            err = tlp_get(gSession2.appHandle, subHandle, &pdInfo, (UINT8 *) data2, &dataSize2);
            if (err == TRDP_NODATA_ERR)
            {
                fprintf(gFp, ".");
                continue;
            }
            
            if (err == TRDP_TIMEOUT_ERR)
            {
                fprintf(gFp, ".");
                fflush(gFp);
                continue;
            }
            
            if (err != TRDP_NO_ERR)
            {
                fprintf(gFp, "\n### tlp_get error: %d\n", err);
                
                gFailed = 1;
                goto end;
                
            }
            else
            {
                fprintf(gFp, "\nreceived data from pull: %s (seq: %u, size: %u)\n", data2, pdInfo.seqCount, dataSize2);
                gFailed = 0;
                goto end;
            }
        }
    }

    /* ------------------------- test code ends here --------------------------- */

    CLEANUP;
}

/**********************************************************************************************************************/
/** test9
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */
static int test9 (int argc, char *argv[])
{
    PREPARE("Send and receive many telegrams, to check time optimisations", "test"); /* allocates appHandle1, appHandle2, failed = 0, err */

    /* ------------------------- test code starts here --------------------------- */

    {
#define TEST9_NO_OF_TELEGRAMS   200u
#define TEST9_COMID     10000u
#define TEST9_INTERVAL  100000u
#define TEST9_TIMEOUT   (TEST9_INTERVAL * 3)
#define TEST9_DATA      "Hello World!"
#define TEST9_DATA_LEN  16u

        TRDP_PUB_T      pubHandle[TEST9_NO_OF_TELEGRAMS];
        TRDP_SUB_T      subHandle[TEST9_NO_OF_TELEGRAMS];
        unsigned int    i;

        for (i = 0; i < TEST9_NO_OF_TELEGRAMS; i++)
        {
            /*    Session1 Install all publishers    */

            err = tlp_publish(gSession1.appHandle, &pubHandle[i], TEST9_COMID + i, 0u, 0u,
                              0u,
                              gSession2.ifaceIP,                              /* Destination */
                              TEST9_INTERVAL,
                              0u, TRDP_FLAGS_DEFAULT, NULL, (UINT8*) TEST9_DATA, TEST9_DATA_LEN);

            IF_ERROR("tlp_publish");
            err = tlp_subscribe(gSession2.appHandle, &subHandle[i], NULL, NULL,
                            TEST9_COMID + i, 0u, 0u,
                            gSession1.ifaceIP,
                            0u,                            /* MC group */
                            TRDP_FLAGS_NONE, TEST9_TIMEOUT, TRDP_TO_DEFAULT);


            IF_ERROR("tlp_subscribe");

        }

        fprintf(gFp, "\nInitialized %u publishers & subscribers!\n", i);

        /*
         Enter the main processing loop.
         */
        int counter = 0;
        while (counter++ < 600)         /* 60 seconds */
        {
            char            data2[1432u];
            UINT32          dataSize2 = sizeof(data2);
            TRDP_PD_INFO_T  pdInfo;


            for (i = 0; i < TEST9_NO_OF_TELEGRAMS; i++)
            {
                sprintf(data2, "--ComId %08u", i);
                (void) tlp_put(gSession1.appHandle, pubHandle[i], (UINT8 *) data2, TEST9_DATA_LEN);

                /*    Wait for answer   */
                usleep(100000u);

                err = tlp_get(gSession2.appHandle, subHandle[i], &pdInfo, (UINT8 *) data2, &dataSize2);
                if (err == TRDP_NODATA_ERR)
                {
                    //fprintf(gFp, ".");
                    continue;
                }

                if (err == TRDP_TIMEOUT_ERR)
                {
                    //fprintf(gFp, ".");
                    //fflush(gFp);
                    continue;
                }

                if (err != TRDP_NO_ERR)
                {
                    fprintf(gFp, "\n### tlp_get error: %d\n", err);

                    gFailed = 1;
                    goto end;

                }
                else
                {
                    //fprintf(gFp, "\nreceived data from pull: %s (seq: %u, size: %u)\n", data2, pdInfo.seqCount, dataSize2);
                    gFailed = 0;
                    //goto end;
                }
            }
        }
    }

    /* ------------------------- test code ends here --------------------------- */

    CLEANUP;
}

/**********************************************************************************************************************/
/** test10
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */
static int test10 (int argc, char *argv[])
{
    PREPARE1(""); /* allocates appHandle1, failed = 0, err = TRDP_NO_ERR */

    /* ------------------------- test code starts here --------------------------- */

    {
        err = 0;
        fprintf(gFp, "TRDP Version %s\n", tlc_getVersionString());
    }

    /* ------------------------- test code ends here --------------------------- */


    CLEANUP;
}

/**********************************************************************************************************************/
/** test11
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */
static int test11 (int argc, char *argv[])
{
    PREPARE("", "test"); /* allocates appHandle1, appHandle2, failed = 0, err */

    /* ------------------------- test code starts here --------------------------- */

    {
    }

    /* ------------------------- test code ends here --------------------------- */


    CLEANUP;
}




/**********************************************************************************************************************/
/** test12
 *
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */
static int test12 (int argc, char *argv[])
{
    /* allocates appHandle1, appHandle2, failed = 0, err = TRDP_NO_ERR */
    PREPARE_COM("");

    /* ------------------------- test code starts here --------------------------- */

    {
    }
    /* ------------------------- test code ends here ------------------------------------ */

    CLEANUP;
}

/**********************************************************************************************************************/
/** test13
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */
static int test13 (int argc, char *argv[])
{
    PREPARE1(""); /* allocates appHandle1, failed = 0, err = TRDP_NO_ERR */
    
    /* ------------------------- test code starts here --------------------------- */
    
    {
        
    }
    
    /* ------------------------- test code ends here --------------------------- */
    
    
    CLEANUP;
}





/**********************************************************************************************************************/
/* This array holds pointers to the m-th test (m = 1 will execute test1...)                                           */
/**********************************************************************************************************************/
test_func_t *testArray[] =
{
    NULL,
    test1,
    test2,
    test3,
    test4,
    test5,
    test6,
    test7,
    test8,
    test9,
    /*test10,
    test11,
    test12,
    test13,*/
    NULL
};

/**********************************************************************************************************************/

/**********************************************************************************************************************/
/** main entry
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */
int main (int argc, char *argv[])
{
    int             ch;
    unsigned int    ip[4];
    UINT32          testNo = 0;

//    if (argc <= 1)
//    {
//        usage(argv[0]);
//        return 1;
//    }

    if (gFp == NULL)
    {
        /* gFp = fopen("/tmp/trdp.log", "w+"); */
        gFp = stdout;
    }

    while ((ch = getopt(argc, argv, "d:i:t:o:m:h?v")) != -1)
    {
        switch (ch)
        {
            case 'o':
            {   /*  read primary ip    */
                if (sscanf(optarg, "%u.%u.%u.%u",
                           &ip[3], &ip[2], &ip[1], &ip[0]) < 4)
                {
                    usage(argv[0]);
                    exit(1);
                }
                gSession1.ifaceIP = (ip[3] << 24) | (ip[2] << 16) | (ip[1] << 8) | ip[0];
                break;
            }
            case 'i':
            {   /*  read alternate ip    */
                if (sscanf(optarg, "%u.%u.%u.%u",
                           &ip[3], &ip[2], &ip[1], &ip[0]) < 4)
                {
                    usage(argv[0]);
                    exit(1);
                }
                gSession2.ifaceIP = (ip[3] << 24) | (ip[2] << 16) | (ip[1] << 8) | ip[0];
                break;
            }
            case 't':
            {   /*  read target ip (MC)   */
                if (sscanf(optarg, "%u.%u.%u.%u",
                           &ip[3], &ip[2], &ip[1], &ip[0]) < 4)
                {
                    usage(argv[0]);
                    exit(1);
                }
                gDestMC = (ip[3] << 24) | (ip[2] << 16) | (ip[1] << 8) | ip[0];
                break;
            }
            case 'm':
            {   /*  read test number    */
                if (sscanf(optarg, "%u",
                           &testNo) < 1)
                {
                    usage(argv[0]);
                    exit(1);
                }
                break;
            }
            case 'v':   /*  version */
                printf("%s: Version %s\t(%s - %s)\n",
                       argv[0], APP_VERSION, __DATE__, __TIME__);
                printf("No. of tests: %lu\n", sizeof(testArray) / sizeof(void *) - 2);
                exit(0);
                break;
            case 'h':
            case '?':
            default:
                usage(argv[0]);
                return 1;
        }
    }

    if (testNo > (sizeof(testArray) / sizeof(void *) - 1))
    {
        printf("%s: test no. %u does not exist\n", argv[0], testNo);
        exit(1);
    }

    if (testNo == 0)    /* Run all tests in sequence */
    {
        while (1)
        {
            int i, ret = 0;
            for (i = 1; testArray[i] != NULL; ++i)
            {
                ret += testArray[i](argc, argv);
            }
            if (ret == 0)
            {
                fprintf(gFp, "All tests passed!\n");
            }
            else
            {
                fprintf(gFp, "### %d test(s) failed! ###\n", ret);
            }
            return ret;
        }
    }

    return testArray[testNo](argc, argv);
}

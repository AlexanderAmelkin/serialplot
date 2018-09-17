/*
  Copyright © 2018 Hasan Yavuz Özderya

  This file is part of serialplot.

  serialplot is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  serialplot is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with serialplot.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <time.h>
#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"

#include "samplepack.h"
#include "source.h"
#include "indexbuffer.h"
#include "linindexbuffer.h"
#include "ringbuffer.h"
#include "readonlybuffer.h"
#include "datachunk.h"
#include "chunkedbuffer.h"

#include "test_helpers.h"

TEST_CASE("samplepack with no X", "[memory]")
{
    SamplePack pack(100, 3, false);

    REQUIRE_FALSE(pack.hasX());
    REQUIRE(pack.numChannels() == 3);
    REQUIRE(pack.numSamples() == 100);

    double* chan0 = pack.data(0);
    double* chan1 = pack.data(1);
    double* chan2 = pack.data(2);

    REQUIRE(chan0 == chan1 - 100);
    REQUIRE(chan1 == chan2 - 100);
}

TEST_CASE("samplepack with X", "[memory]")
{
    SamplePack pack(100, 3, true);

    REQUIRE(pack.hasX());
    REQUIRE(pack.numChannels() == 3);
    REQUIRE(pack.numSamples() == 100);
    REQUIRE(pack.xData() != nullptr);
}

TEST_CASE("samplepack copy", "[memory]")
{
    SamplePack pack(10, 3, true);

    // fill test data
    for (int i = 0; i < 10; i++)
    {
        pack.xData()[i] = i;
        pack.data(0)[i] = i+5;
        pack.data(1)[i] = i*2;
        pack.data(2)[i] = i*3;
    }

    SamplePack other = pack;
    // compare
    for (int i = 0; i < 10; i++)
    {
        REQUIRE(other.xData()[i] == i);
        REQUIRE(other.data(0)[i] == i+5);
        REQUIRE(other.data(1)[i] == i*2);
        REQUIRE(other.data(2)[i] == i*3);
    }
}

TEST_CASE("sink", "[memory, stream]")
{
    TestSink sink;
    SamplePack pack(100, 3, false);

    sink.setNumChannels(3, false);
    REQUIRE(sink.numChannels() == 3);

    sink.feedIn(pack);
    REQUIRE(sink.totalFed == 100);
    sink.feedIn(pack);
    REQUIRE(sink.totalFed == 200);

    TestSink follower;

    sink.connectFollower(&follower);
    REQUIRE(follower.numChannels() == 3);
    REQUIRE(follower.hasX() == false);

    sink.feedIn(pack);
    REQUIRE(sink.totalFed == 300);
    REQUIRE(follower.totalFed == 100);

    sink.setNumChannels(2, true);
    REQUIRE(follower.numChannels() == 2);
    REQUIRE(follower.hasX() == true);
}

TEST_CASE("sink must be created unconnected", "[memory, stream]")
{
    TestSink sink;
    REQUIRE(sink.connectedSource() == NULL);
}

TEST_CASE("source", "[memory, stream]")
{
    TestSink sink;

    TestSource source(3, false);

    REQUIRE(source.numChannels() == 3);
    REQUIRE(source.hasX() == false);

    source.connectSink(&sink);
    REQUIRE(sink.numChannels() == 3);
    REQUIRE(sink.hasX() == false);

    source._setNumChannels(5, true);
    REQUIRE(sink.numChannels() == 5);
    REQUIRE(sink.hasX() == true);

    SamplePack pack(100, 5, true);
    source._feed(pack);
    REQUIRE(sink.totalFed == 100);

    source.disconnect(&sink);
    source._feed(pack);
    REQUIRE(sink.totalFed == 100);
}

TEST_CASE("source must set/unset sink 'source'", "[memory, stream]")
{
    TestSink sink;
    TestSource source(3, false);

    source.connectSink(&sink);
    REQUIRE(sink.connectedSource() == &source);

    source.disconnect(&sink);
    REQUIRE(sink.connectedSource() == NULL);
}

TEST_CASE("source disconnect all sinks", "[memory, stream]")
{
    TestSink sinks[3];
    TestSource source(3, false);

    // connect sinks
    for (int i = 0; i < 3; i++)
    {
        source.connectSink(&sinks[i]);
    }

    source.disconnectSinks();
    for (int i = 0; i < 3; i++)
    {
        REQUIRE(sinks[i].connectedSource() == NULL);
    }
}

TEST_CASE("IndexBuffer", "[memory, buffer]")
{
    IndexBuffer buf(10);

    REQUIRE(buf.size() == 10);
    for (unsigned i = 0; i < 10; i++)
    {
        REQUIRE(buf.sample(i) == i);
    }
    auto l = buf.limits();
    REQUIRE(l.start == 0);
    REQUIRE(l.end == 9);

    buf.resize(20);
    REQUIRE(buf.size() == 20);
    REQUIRE(buf.sample(15) == 15);
    l = buf.limits();
    REQUIRE(l.start == 0);
    REQUIRE(l.end == 19);
}

TEST_CASE("LinIndexBuffer", "[memory, buffer]")
{
    LinIndexBuffer buf(10, 0., 3.0);

    REQUIRE(buf.size() == 10);
    REQUIRE(buf.sample(0) == 0.);
    REQUIRE(buf.sample(9) == 3.0);
    REQUIRE(buf.sample(4) == Approx(1+1/3.));

    auto l = buf.limits();
    REQUIRE(l.start == 0.);
    REQUIRE(l.end == 3.);

    buf.resize(20);
    REQUIRE(buf.size() == 20);
    REQUIRE(buf.sample(0) == 0.);
    REQUIRE(buf.sample(9) == Approx(9.*3./19.));
    REQUIRE(buf.sample(4) == Approx(4.*3./19.));
    REQUIRE(buf.sample(19) == 3.0);

    l = buf.limits();
    REQUIRE(l.start == 0.);
    REQUIRE(l.end == 3.0);

    buf.setLimits({-5., 5.});
    l = buf.limits();
    REQUIRE(l.start == -5.0);
    REQUIRE(l.end == 5.0);

    REQUIRE(buf.sample(0) == -5.0);
    REQUIRE(buf.sample(19) == 5.0);
}

TEST_CASE("RingBuffer sizing", "[memory, buffer]")
{
    RingBuffer buf(10);

    REQUIRE(buf.size() == 10);

    buf.resize(5);
    REQUIRE(buf.size() == 5);

    buf.resize(15);
    REQUIRE(buf.size() == 15);
}

TEST_CASE("RingBuffer initial values should be 0", "[memory, buffer]")
{
    RingBuffer buf(10);

    for (unsigned i = 0; i < 10; i++)
    {
        REQUIRE(buf.sample(i) == 0.);
    }
}

TEST_CASE("RingBuffer data access", "[memory, buffer]")
{
    RingBuffer buf(10);
    double values[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    buf.addSamples(values, 10);

    REQUIRE(buf.size() == 10);
    for (unsigned i = 0; i < 10; i++)
    {
        REQUIRE(buf.sample(i) == values[i]);
    }

    buf.addSamples(values, 5);

    REQUIRE(buf.size() == 10);
    for (unsigned i = 0; i < 5; i++)
    {
        REQUIRE(buf.sample(i) == values[i+5]);
    }
    for (unsigned i = 5; i < 10; i++)
    {
        REQUIRE(buf.sample(i) == values[i-5]);
    }
}

TEST_CASE("making RingBuffer bigger should keep end values", "[memory, buffer]")
{
    RingBuffer buf(5);
    double values[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    buf.addSamples(values, 5);
    buf.resize(10);

    REQUIRE(buf.size() == 10);
    for (unsigned i = 0; i < 5; i++)
    {
        REQUIRE(buf.sample(i) == 0);
    }
    for (unsigned i = 5; i < 10; i++)
    {
        REQUIRE(buf.sample(i) == values[i-5]);
    }
}

TEST_CASE("making RingBuffer smaller should keep end values", "[memory, buffer]")
{
    RingBuffer buf(10);
    double values[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    buf.addSamples(values, 10);
    buf.resize(5);

    REQUIRE(buf.size() == 5);
    for (unsigned i = 0; i < 5; i++)
    {
        REQUIRE(buf.sample(i) == values[i+5]);
    }
}

TEST_CASE("RingBuffer limits", "[memory, buffer]")
{
    RingBuffer buf(10);
    double values[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    auto lim = buf.limits();
    REQUIRE(lim.start == 0.);
    REQUIRE(lim.end == 0.);

    buf.addSamples(values, 10);
    lim = buf.limits();
    REQUIRE(lim.start == 1.);
    REQUIRE(lim.end == 10.);

    buf.addSamples(&values[9], 1);
    lim = buf.limits();
    REQUIRE(lim.start == 2.);
    REQUIRE(lim.end == 10.);

    buf.addSamples(values, 9);
    buf.addSamples(values, 1);
    lim = buf.limits();
    REQUIRE(lim.start == 1.);
    REQUIRE(lim.end == 9.);
}

TEST_CASE("RingBuffer clear", "[memory, buffer]")
{
    RingBuffer buf(10);
    double values[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    buf.addSamples(values, 10);
    buf.clear();

    REQUIRE(buf.size() == 10);
    for (unsigned i = 0; i < 10; i++)
    {
        REQUIRE(buf.sample(i) == 0.);
    }
    auto lim = buf.limits();
    REQUIRE(lim.start == 0.);
    REQUIRE(lim.end == 0.);
}

TEST_CASE("ReadOnlyBuffer", "[memory, buffer]")
{
    IndexBuffer source(10);

    ReadOnlyBuffer buf(&source);

    REQUIRE(buf.size() == 10);
    auto lim = buf.limits();
    REQUIRE(lim.start == 0.);
    REQUIRE(lim.end == 9.);
    for (unsigned i = 0; i < 10; i++)
    {
        REQUIRE(buf.sample(i) == i);
    }
}

TEST_CASE("ReadOnlyBuffer sliced constructor", "[memory, buffer]")
{
    IndexBuffer source(10);

    ReadOnlyBuffer buf(&source, 5, 4);

    REQUIRE(buf.size() == 4);
    auto lim = buf.limits();
    REQUIRE(lim.start == 5.);
    REQUIRE(lim.end == 8.);
    for (unsigned i = 0; i < 4; i++)
    {
        REQUIRE(buf.sample(i) == (i + 5));
    }
}

TEST_CASE("DataChunk created empty", "[memory]")
{
    DataChunk c(0, 1000);
    REQUIRE(c.size() == 0);
    REQUIRE(c.capacity() == 1000);
    REQUIRE(c.start() == 0);
    REQUIRE(c.end() == 0);
    REQUIRE_FALSE(c.isFull());
    REQUIRE(c.left() == 1000);
}

TEST_CASE("adding data to DataChunk", "[memory]")
{
    DataChunk c(0, 1000);
    double samples[10] = {1,2,3,4,5,6,7,8,9,10};
    c.addSamples(samples, 10);

    REQUIRE(c.size() == 10);
    REQUIRE(c.capacity() == 1000);
    REQUIRE(c.start() == 0);
    REQUIRE(c.end() == 10);
    REQUIRE(c.left() == 990);

    for (int i = 0; i < 10; i++)
    {
        REQUIRE(c.sample(i) == samples[i]);
    }

    REQUIRE(c.min() == 1);
    REQUIRE(c.max() == 10);
    REQUIRE(c.avg() == Approx(5.5));
    REQUIRE(c.meanSquare() == Approx(38.5));
}

TEST_CASE("filling data chunk", "[memory]")
{
    DataChunk c(0, 1000);

    for (int i = 0; i < 1000; i++)
    {
        double sample = i + 1;
        c.addSamples(&sample, 1);
    }

    REQUIRE(c.size() == 1000);
    REQUIRE(c.capacity() == 1000);
    REQUIRE(c.start() == 0);
    REQUIRE(c.end() == 1000);
    REQUIRE(c.left() == 0);

    REQUIRE(c.min() == 1);
    REQUIRE(c.max() == 1000);
    REQUIRE(c.avg() == Approx(500.5));
    REQUIRE(c.meanSquare() == Approx(333833.5));
}

TEST_CASE("ChunkedBuffer created empty", "[memory]")
{
    ChunkedBuffer b;

    REQUIRE(b.size() == 0);
    REQUIRE(b.boundingRect() == QRectF(0,0,0,0));
}

TEST_CASE("ChunkedBuffer adding data and clearing", "[memory]")
{
    ChunkedBuffer b;

    // add some small data
    const int N = 10;
    double samples[N] = {1,2,3,4,5,6,7,8,9,10};
    b.addSamples(samples, N);

    REQUIRE(b.size() == N);
    REQUIRE(b.boundingRect() == QRectF(0,10,N,9));

    // add data to fill the chunk
    double samples2[CHUNK_SIZE-N] = {0};
    b.addSamples(samples2, CHUNK_SIZE-N);
    REQUIRE(b.size() == CHUNK_SIZE);
    REQUIRE(b.boundingRect() == QRectF(0,10,CHUNK_SIZE,10));

    // add data for second chunk
    b.addSamples(samples, N);
    REQUIRE(b.size() == CHUNK_SIZE+N);
    REQUIRE(b.boundingRect() == QRectF(0,10,CHUNK_SIZE+N,10));

    // add more data to make it 4 chunks
    double samples3[CHUNK_SIZE*3-N] = {0};
    b.addSamples(samples3, CHUNK_SIZE*3-N);
    REQUIRE(b.size() == CHUNK_SIZE*4);
    REQUIRE(b.boundingRect() == QRectF(0,10,CHUNK_SIZE*4,10));

    // clear
    b.clear();
    REQUIRE(b.size() == 0);
}

TEST_CASE("ChunkedBuffer accessing data", "[memory]")
{
    ChunkedBuffer b;

    // prepare data
    double samples[CHUNK_SIZE*3];
    samples[0] = 10;
    samples[10] = 20;
    samples[CHUNK_SIZE-1] = 30;
    samples[CHUNK_SIZE] = 40;
    samples[CHUNK_SIZE+1] = 50;
    samples[CHUNK_SIZE*2-1] = 60;
    samples[CHUNK_SIZE*3-1] = 70;

    // test
    b.addSamples(samples, CHUNK_SIZE*3);

    REQUIRE(b.size() == CHUNK_SIZE*3);
    REQUIRE(b.sample(0) == 10);
    REQUIRE(b.sample(10) == 20);
    REQUIRE(b.sample(CHUNK_SIZE-1) == 30);
    REQUIRE(b.sample(CHUNK_SIZE) == 40);
    REQUIRE(b.sample(CHUNK_SIZE+1) == 50);
    REQUIRE(b.sample(CHUNK_SIZE*2-1) == 60);
    REQUIRE(b.sample(CHUNK_SIZE*3-1) == 70);
}

TEST_CASE("ChunkedBuffer time measurement", "[.][timing][memory]")
{
    const int N = CHUNK_SIZE*10;
    clock_t start, end;
    double samples[N];
    ChunkedBuffer b;
    start = clock();
    b.addSamples(samples, N);
    end = clock();
    REQUIRE(b.size() == N);
    WARN("addSamples(" << N << ") took: " << ((end-start) / ((double) CLOCKS_PER_SEC)));

    // access
    start = clock();
    for (int i =0; i < N; i++)
    {
        samples[i] = b.sample(i);
    }
    end = clock();
    WARN("sample()*"<< N <<" took: " << ((end-start) / ((double) CLOCKS_PER_SEC)));
}

#define BOOST_TEST_MAIN TestCompressedSparse
//#define DEBUG
#include <boost/test/unit_test.hpp>
#include <stdexcept>
#include <string>
#include <vector>
#include <zmq.hpp>
#include <thread>
#include <atomic>
#include <sam/tuples/Edge.hpp>
#include <sam/tuples/Tuplizer.hpp>
#include <sam/tuples/VastNetflow.hpp>
#include <sam/tuples/VastNetflowGenerators.hpp>
#include <sam/CompressedSparse.hpp>
#include <sam/Util.hpp>

using namespace sam;
using namespace sam::vast_netflow;

typedef Edge<size_t, EmptyLabel, VastNetflow> EdgeType;
typedef CompressedSparse<EdgeType, 
   DestIp, SourceIp, TimeSeconds, DurationSeconds, 
   StringHashFunction, StringEqualityFunction> GraphType;
typedef TuplizerFunction<EdgeType, MakeVastNetflow> Tuplizer;

BOOST_AUTO_TEST_CASE( test_compressed_sparse_one_vertex )
{
  /** 
   * Tests when we have only one source vertex.
   */
  size_t capacity = 1000;
  double window = 1000; //Make big window so we don't lose anything
  auto graph = new GraphType(capacity, window); 
    
  int numThreads = 100;
  int numExamples = 1000;
  auto id = new std::atomic<int>(0);


  std::vector<std::thread> threads;
  for (int i = 0; i < numThreads; i++) {
    threads.push_back(std::thread([graph, id, i, numExamples]() {

      UniformDestPort generator("192.168.0.1", 1);

      Tuplizer tuplizer;
      for (int j =0; j < numExamples; j++) {
        size_t my_id = id->fetch_add(1);
        EdgeType edge = tuplizer(my_id, generator.generate());
        graph->addEdge(edge);
      }
    }));

  }

  for (int i = 0; i < numThreads; i++) {
    threads[i].join();
  }

  size_t count = graph->countEdges();
  BOOST_CHECK_EQUAL(count, numThreads * numExamples);
}

BOOST_AUTO_TEST_CASE( test_compressed_sparse_many_vertices )
{
  /** 
   * Tests when we have lots of source vertices (sourc ips)
   */
  size_t capacity = 1000;
  double window = 1000; //Make big window so we don't lose anything
  auto graph = new GraphType(capacity, window); 
             
  int numThreads = 100;
  int numExamples = 1000;
  auto id = new std::atomic<int>(0);


  std::vector<std::thread> threads;
  for (int i = 0; i < numThreads; i++) {
    threads.push_back(std::thread([graph, id, i, numExamples]() {

      UniformDestPort generator("192.168.0.1", 1);
      Tuplizer tuplizer;
      for (int j =0; j < numExamples; j++) {
        size_t my_id = id->fetch_add(1);
        EdgeType edge = tuplizer(my_id, generator.generate());
        graph->addEdge(edge);
      }
       

    }));

  }

  for (int i = 0; i < numThreads; i++) {
    threads[i].join();
  }

  size_t count = graph->countEdges();
  std::cout << "count " << count << std::endl;
  std::cout << "num examples " << numExamples << " numThreads " 
            << numThreads << std::endl;
  BOOST_CHECK_EQUAL(count, numThreads * numExamples);
}

BOOST_AUTO_TEST_CASE( test_compressed_sparse_small_capacity )
{
  /**
   * This tests adding a bunch of edges when the capacity is just 1, so
   * we make sure that even if the capacity is smaller than the number of 
   * source vertices, it can still handle it.
   */
  size_t capacity = 1;
  double window = 1000; //Make big window so we don't lose anything
  auto graph = new GraphType(capacity, window); 

  int numThreads = 100;
  int numExamples = 1;
  auto id = new std::atomic<int>(0);

  std::vector<std::thread> threads;
  for (int i = 0; i < numThreads; i++) {
    threads.push_back(std::thread([graph, id, i, numExamples]() {

      //UniformDestPort generator("192.168.0.1", 1);
      UniformDestPort generator("192.168.0." + 
        boost::lexical_cast<std::string>(i), 1);

      Tuplizer tuplizer;
      for (int j =0; j < numExamples; j++) {
        size_t my_id = id->fetch_add(1);
        EdgeType edge = tuplizer(my_id, generator.generate());
        graph->addEdge(edge);
      }
       

    }));

  }

  for (int i = 0; i < numThreads; i++) {
    threads[i].join();
  }

  size_t count = graph->countEdges();

  std::cout << "count " << count << std::endl;
  std::cout << "num examples " << numExamples << " numThreads " 
            << numThreads << std::endl;
  BOOST_CHECK_EQUAL(count, numThreads * numExamples);
}

BOOST_AUTO_TEST_CASE( test_work )
{
  /// Adding the first edge should be one unit of work
  size_t capacity = 1;
  double window = .00000000001; //Make small window
  auto graph = new GraphType(capacity, window); 

  UniformDestPort generator("192.168.0." + 
    boost::lexical_cast<std::string>(1), 1);

  Tuplizer tuplizer;
  EdgeType edge = tuplizer(0, generator.generate());
  size_t work = graph->addEdge(edge);
  BOOST_CHECK_EQUAL(work, 1);

}

BOOST_AUTO_TEST_CASE( test_cleanup )
{
  /**
   * This tests cleaning up edges when the window has passed. 
   */
  size_t capacity = 1;
  double window = .00000000001; //Make small window
  //double window = .1; //Make small window
  auto graph = new GraphType(capacity, window); 

  int numThreads = 10;
  int numExamples = 10000;
  auto id = new std::atomic<int>(0);
  auto work = new std::atomic<size_t>(0);

  std::vector<std::thread> threads;
  for (int i = 0; i < numThreads; i++) {
    threads.push_back(std::thread([graph, id, i, numExamples, work]() {

      //UniformDestPort generator("192.168.0.1", 1); 
      UniformDestPort generator("192.168.0." + 
        boost::lexical_cast<std::string>(i), 1);

      Tuplizer tuplizer;
      for (int j =0; j < numExamples; j++) {
        EdgeType edge = tuplizer(id->fetch_add(1), generator.generate());
        work->fetch_add(graph->addEdge(edge));
      }
       
    }));
  }

  for (int i = 0; i < numThreads; i++) {
    threads[i].join();
  }

  size_t count = graph->countEdges();
  // Not sure how to make this exact, but almost all of the edges should
  // be deleted because the window is so small.
  // TODO
  std::cout << "count " << count << " numThreads " << numThreads << std::endl;
  BOOST_CHECK(count <  numThreads * numExamples * 0.1);

  // Since the capacity is 1, all of the edges should go to the same list
  // in alle, so the process should be add edge (1 work unit) and delete old
  // edge (1 work unit).  So the total amount of work should be close to 
  // 2 * number of edges added.
  //std::cout << "work " << work->load() << " 2 * numExamples * numThreads "
  //          << 2 * numExamples * numThreads << std::endl;
  //BOOST_CHECK(work->load() < 2 * numExamples * numThreads * 10);
  //BOOST_CHECK(work->load() > 2 * numExamples * numThreads * 10 - numThreads);
}


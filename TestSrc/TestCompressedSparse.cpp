#define BOOST_TEST_MAIN TestCompressedSparse
#include <boost/test/unit_test.hpp>
#include <stdexcept>
#include <string>
#include <vector>
#include <zmq.hpp>
#include <thread>
#include <atomic>
#include "Edge.hpp"
#include "Netflow.hpp"
#include "NetflowGenerators.hpp"
#include "CompressedSparse.hpp"
#include "Util.hpp"

using namespace sam;

BOOST_AUTO_TEST_CASE( test_compressed_sparse_one_vertex )
{
  /** 
   * Tests when we have only one source vertex.
   */
  size_t capacity = 1000;
  double window = 1000; //Make big window so we don't lose anything
  auto graph = new CompressedSparse<Netflow, 
   DestIp, SourceIp, TimeSeconds, 
   StringHashFunction, StringEqualityFunction>(capacity, window); 
    

  int numThreads = 100;
  int numExamples = 1000;
  auto id = new std::atomic<int>(0);


  std::vector<std::thread> threads;
  for (int i = 0; i < numThreads; i++) {
    threads.push_back(std::thread([graph, id, i, numExamples]() {

      UniformDestPort generator("192.168.0.1", 1);
      //UniformDestPort generator("192.168.0." + 
      //  boost::lexical_cast<std::string>(i), 1);

      for (int j =0; j < numExamples; j++) {
        Netflow netflow = makeNetflow(id->fetch_add(1), generator.generate());
        graph->addEdge(netflow);
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
  auto graph = new CompressedSparse<Netflow, 
             SourceIp, DestIp, TimeSeconds, 
             StringHashFunction, StringEqualityFunction>(capacity, window); 
    

  int numThreads = 100;
  int numExamples = 1000;
  auto id = new std::atomic<int>(0);


  std::vector<std::thread> threads;
  for (int i = 0; i < numThreads; i++) {
    threads.push_back(std::thread([graph, id, i, numExamples]() {

      UniformDestPort generator("192.168.0.1", 1);
      //UniformDestPort generator("192.168.0." + 
      //  boost::lexical_cast<std::string>(i), 1);

      for (int j =0; j < numExamples; j++) {
        Netflow netflow = makeNetflow(id->fetch_add(1), generator.generate());
        graph->addEdge(netflow);
      }
       

    }));

  }

  for (int i = 0; i < numThreads; i++) {
    threads[i].join();
  }

  size_t count = graph->countEdges();
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
  auto graph = new CompressedSparse<Netflow, 
             SourceIp, DestIp, TimeSeconds, 
             StringHashFunction, StringEqualityFunction>(capacity, window); 
    

  int numThreads = 100;
  int numExamples = 1;
  auto id = new std::atomic<int>(0);

  std::vector<std::thread> threads;
  for (int i = 0; i < numThreads; i++) {
    threads.push_back(std::thread([graph, id, i, numExamples]() {

      //UniformDestPort generator("192.168.0.1", 1);
      UniformDestPort generator("192.168.0." + 
        boost::lexical_cast<std::string>(i), 1);

      for (int j =0; j < numExamples; j++) {
        Netflow netflow = makeNetflow(id->fetch_add(1), generator.generate());
        graph->addEdge(netflow);
      }
       

    }));

  }

  for (int i = 0; i < numThreads; i++) {
    threads[i].join();
  }

  size_t count = graph->countEdges();
  BOOST_CHECK_EQUAL(count, numThreads * numExamples);
}

BOOST_AUTO_TEST_CASE( test_cleanup )
{
  /**
   * This tests cleaning up edges when the window has passed. 
   */
  size_t capacity = 1;
  double window = .00000000001; //Make small window
  //double window = .1; //Make small window
  auto graph = new CompressedSparse<Netflow, 
             DestIp, SourceIp, TimeSeconds, 
             StringHashFunction, StringEqualityFunction>(capacity, window); 
    

  int numThreads = 10;
  int numExamples = 10000;
  auto id = new std::atomic<int>(0);

  std::vector<std::thread> threads;
  for (int i = 0; i < numThreads; i++) {
    threads.push_back(std::thread([graph, id, i, numExamples]() {

      //UniformDestPort generator("192.168.0.1", 1); 
      UniformDestPort generator("192.168.0." + 
        boost::lexical_cast<std::string>(i), 1);

      for (int j =0; j < numExamples; j++) {
        Netflow netflow = makeNetflow(id->fetch_add(1), generator.generate());
        graph->addEdge(netflow);
      }
       
    }));

  }

  for (int i = 0; i < numThreads; i++) {
    threads[i].join();
  }

  size_t count = graph->countEdges();
  // Not sure how to make this exact, but almost all of the edges should
  // be deleted because the window is so small.
  std::cout << "count " << count << std::endl;
  BOOST_CHECK(count <  numThreads * 5);
}


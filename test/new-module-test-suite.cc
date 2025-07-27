// Include a header file from your module to test.
#include "ns3/ipv4-drill-routing-protocol.h"

// An essential include is test.h
#include "ns3/test.h"

// Do not put your test classes in namespace ns3.  You may find it useful
// to use the using directive to access the ns3 namespace directly
using namespace ns3;

// Add a doxygen group for tests.
// If you have more than one test, this should be in only one of them.
/**
 * @defgroup new-module-tests Tests for new-module
 * @ingroup new-module
 * @ingroup tests
 */

// This is an example TestCase.
/**
 * @ingroup new-module-tests
 * Test case for feature 1
 */
class NewModuleTestCase1 : public TestCase
{
  public:
    NewModuleTestCase1();
    ~NewModuleTestCase1() override;

  private:
    void DoRun() override;
};

// Add some help text to this case to describe what it is intended to test
NewModuleTestCase1::NewModuleTestCase1()
    : TestCase("NewModule test case (does nothing)")
{
}

// This destructor does nothing but we include it as a reminder that
// the test case should clean up after itself
NewModuleTestCase1::~NewModuleTestCase1()
{
}

//
// This method is the pure virtual method from class TestCase that every
// TestCase must implement
//
void
NewModuleTestCase1::DoRun()
{
    // A wide variety of test macros are available in src/core/test.h
    NS_TEST_ASSERT_MSG_EQ(true, true, "true doesn't equal true for some reason");
    // Use this one for floating point comparisons
    NS_TEST_ASSERT_MSG_EQ_TOL(0.01, 0.01, 0.001, "Numbers are not equal within tolerance");
}

// The TestSuite class names the TestSuite, identifies what type of TestSuite,
// and enables the TestCases to be run.  Typically, only the constructor for
// this class must be defined

/**
 * @ingroup new-module-tests
 * TestSuite for module new-module
 */
class NewModuleTestSuite : public TestSuite
{
  public:
    NewModuleTestSuite();
};

// Type for TestSuite can be UNIT, SYSTEM, EXAMPLE, or PERFORMANCE
NewModuleTestSuite::NewModuleTestSuite()
    : TestSuite("new-module", Type::UNIT)
{
    // Duration for TestCase can be QUICK, EXTENSIVE or TAKES_FOREVER
    AddTestCase(new NewModuleTestCase1, TestCase::Duration::QUICK);
}

// Do not forget to allocate an instance of this TestSuite
/**
 * @ingroup new-module-tests
 * Static variable for test initialization
 */
static NewModuleTestSuite snewModuleTestSuite;

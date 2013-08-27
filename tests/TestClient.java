import java.util.logging.Level;
import java.util.Random;
import com.mellanox.EventQueueHandler;
import com.mellanox.JXLog;


public class TestClient {
	
	// Client Parameters
	public static String hostname;
	public static int port;
	public static int portRange = 1234;
	// General Parameters
	private static JXLog testLog = JXLog.getLog(TestClient.class.getCanonicalName());
	private static int requestedTest;
	public static int numberOfTests = 4;
	private static boolean[] successIndicators = new boolean[numberOfTests];
	
	public static void main(String[] args) {
		
		// Check arguments
		if (! argsCheck(args)){
			return;
		} else {
			
			// Get Hostname and Port
			hostname = args[0];
			port = Integer.parseInt(args[1]);
			// Get requested tests
			requestedTest = Integer.parseInt(args[2]);
			
			print("*** Starting a Session Client Test ***");
			// Setting up and Event Queue Handler
			print("----- Setting up and Event Queue Handler...");
			int size = 1000; // TODO Why does it even need a size if it is not used?
			MyEQH eqh = new MyEQH(size);
			// Run Tests
			switch(requestedTest){
			case 0: clientTest1(eqh);
					clientTest2(eqh);
					clientTest3(eqh);
					clientTest4();
					report();
					return;
			case 1:	clientTest1(eqh);
					report();
					return;
			case 2:	clientTest2(eqh);
					report();
					return;
			case 3: clientTest3(eqh);
					report();
					return;
			case 4: clientTest4();
					report();
					return;
			default: print("[TEST ERROR] Unknow test number.");
					return;
			}
		}
	}
	
	private static void clientTest1(MyEQH eqh){
		///////////////////// Test 1 /////////////////////
		// Open and close a client 
		print("*** Test 1: Open and close a client *** ");
		
		// Setup parameters
		MySesClient sClient;
		String url;
		
		// Get url
		url = "rdma://" + hostname + ":" + port;
		
		// Setting up a session client
		print("----- Setting up a session client...");
		sClient = new MySesClient(eqh, url);
		
		// Closing the session client
		print("------ Closing the session client...");
		sClient.close();

		successIndicators[0] = true;
		print("*** Test 1 Passed! *** ");
	}
	
	private static void clientTest2(MyEQH eqh){
		///////////////////// Test 2 /////////////////////
		// A non existing IP address
		print("*** Test 2: A non existing IP address *** ");
		
		// Setup parameters
		MySesClient sClient;
		String url;
		
		// Get url
		url = "rdma://" + "0.0.0.0" + ":" + port;
		
		// Setting up a session client
		print("----- Setting up a session client...");
		sClient = new MySesClient(eqh, url);
		
		// Closing the session client
		print("------ Closing the session client...");
		sClient.close();

		successIndicators[1] = true;
		print("*** Test 2 Passed! *** ");
	}
	
	private static void clientTest3(MyEQH eqh){
		///////////////////// Test 3 /////////////////////
		// Multiple session client on the same EQH
		print("*** Test 3: Multiple session client on the same EQH *** ");
		
		// Setup Multiple Clients Parameters
		MySesClient[] sClientArray;
		int numOfSessionClients = 3;
		String url;
		
		// Setting up a multiple session clients
		print("----- Setting up a multiple session clients...");
		Random portGenerator = new Random();
		sClientArray = new MySesClient[numOfSessionClients];
		for (int i = 0; i < numOfSessionClients; i++){
			// Rnadomize Port
			port = portGenerator.nextInt(portRange) + 1;
			
			// Get url
			url = "rdma://" + hostname + ":" + port;
			
			sClientArray[i] = new MySesClient(eqh, url);
		}
		
		// Closing the session clients
		print("------ Closing the session client...");
		for (int i = 0; i < numOfSessionClients; i++){
			sClientArray[i].close();
		}
		
		successIndicators[2] = true;
		print("*** Test 3 Passed! *** ");
	}
	
	private static void clientTest4(){
		///////////////////// Test 4 /////////////////////
		// Multipule threads on the same EQH
		print("*** Test 4: Multipule threads on the same EQH*** ");
		
		// Setup parameters
		String url;
		
		// Get url
		url = "rdma://" + hostname + ":" + port;
		
		TestClient tc = new TestClient();
		MyThread t1 = tc.new MyThread("t1", new MyEQH(1000), url);
		MyThread t2 = tc.new MyThread("t2", new MyEQH(1000), url);
		MyThread t3 = tc.new MyThread("t3", new MyEQH(1000), url);
		MyThread t4 = tc.new MyThread("t4", new MyEQH(1000), url);
		MyThread t5 = tc.new MyThread("t5", new MyEQH(1000), url);
		MyThread t6 = tc.new MyThread("t6", new MyEQH(1000), url);
		
		t1.start();
		t2.start();
		t3.start();
		t4.start();
		t5.start();
		t6.start();
		
		// Wait for theard to end
		try{
			t1.join();
			t2.join();
			t3.join();
			t4.join();
			t5.join();
			t6.join();
		} catch (InterruptedException e){
			
		}
		
		successIndicators[3] = true;
		print("*** Test 4 Passed! *** ");
	}
	
	class MyThread extends Thread{
		
		EventQueueHandler eqh;
		String url;
		MySesClient sClient;
		
		public MyThread(String caption, EventQueueHandler eqh, String url) {
			super(caption);
			this.eqh = eqh;
			this.url = url;
		}
		
		public void run(){
			// Setting up a session client
			print("----- Setting up a session client...");
			sClient = new MySesClient(eqh, url);
			
			// Wait
			try{
				sleep((long)(Math.random()*1000));
			} catch (InterruptedException e){
				
			}
			
			// Closing the session client
			print("------ Closing the session client...");
			sClient.close();
		}
	}
	
	private static boolean argsCheck(String[] args){
		if (args.length <= 0){
			print("[TEST ERROR] Missing arguments.");
			usage();
			return false;
		} else if (args.length < 3){
			usage();
			return false;
		}
		return true;
	}
	
	private static void report(){
		String passed;
		String report = "Tests Report:\n=============\n";
		for (int i = 0; i < numberOfTests; i++){
			passed = successIndicators[i] ? "Passed" : "Failed";
			report += "Test " + (i+1) + " " + passed + "!\n";
		}
		print(report);
	}
	
	public static void usage(){
		print("Usage: ./runClientTest.sh <HOSTNAME> <PORT> [test]\nWhere [test] includes:\n0		Run all tests \n<n>		Run test number <n>\n");
	}
	
	private static void print(String str){
		System.out.println("\n" + str + "\n");
		//testLog.log(Level.INFO, str);
	}

}
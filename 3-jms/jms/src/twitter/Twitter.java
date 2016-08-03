package twitter;

import org.exolab.jms.administration.AdminConnectionFactory;
import org.exolab.jms.administration.JmsAdminServerIfc;

import javax.jms.*;
import javax.naming.Context;
import javax.naming.InitialContext;
import javax.naming.NamingException;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.net.MalformedURLException;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.Vector;

public class Twitter {

    public static LinkedList<Tweet> tweets = new LinkedList<>();
    public static HashMap<String, MessageConsumer> subscribedChannelsMap = new HashMap<>();
    public static Integer newTweetsCount = 0;

    public static boolean topicExists(String name) {
        boolean result = true;
        String url = "tcp://localhost:3035/";
        try {
            JmsAdminServerIfc admin = AdminConnectionFactory.create(url);
            if (!admin.destinationExists(name)) {
                result = false;
            }
            admin.close();
        } catch (JMSException | MalformedURLException e) {
            e.printStackTrace();
            return false;
        }
        return result;
    }

    public static boolean createDestination(String name, boolean isQueue) { //false = topic, true = queue
        String url = "tcp://localhost:3035/";
        try {
            JmsAdminServerIfc admin = AdminConnectionFactory.create(url);
            if (!admin.destinationExists(name)) {
                if (!admin.addDestination(name, isQueue)) {
                    System.err.println("Failed to create queue/topic " + name);
                    return false;
                }
            }
            admin.close();
        } catch (JMSException | MalformedURLException e) {
            e.printStackTrace();
            return false;
        }
        return true;
    }

    public static Topic createTopic(String topicName, Context context) throws NamingException {
        if (createDestination(topicName, false))
            return (Topic) context.lookup(topicName);
        return null;
    }

    /*public static Queue createQueue(String queueName, Context context) throws NamingException {
        if (createDestination(queueName, true))
            return (Queue) context.lookup(queueName);
        return null;
    }*/

    public static void printMenu(String myTopicName) {
        System.out.println("############################################################");
        System.out.print("new: (" + newTweetsCount + ") ");
        int count = newTweetsCount.toString().length() + myTopicName.length();
        for (int i = 0; i < 60-count-9; i++)
            System.out.print("#");
        System.out.println(" " + myTopicName);
        System.out.println("########################### MENU ###########################");
        System.out.println("[1] write new tweet");
        System.out.println("[2] show recent tweets from subscribed channels");
        System.out.println("[3] show all channels");
        System.out.println("[4] show subscribed channels");
        System.out.println("[5] subscribe new channel");
        System.out.println("[6] unsubscribe channel");
        System.out.println();
        System.out.println("[q] QUIT");
        System.out.println();
    }

    public static void printAllChannels() {
        System.out.println("\n  All channels: ");
        String url = "tcp://localhost:3035/";
        try {
            JmsAdminServerIfc admin = AdminConnectionFactory.create(url);
            Vector destinations = admin.getAllDestinations();
            for (Object d: destinations) {
                if (d instanceof Topic) {
                    Topic topic = (Topic) d;
                    System.out.println(topic.getTopicName());
                }
            }
            admin.close();
        } catch (JMSException | MalformedURLException e) {
            e.printStackTrace();
        }
    }

    public static void writeTweet(Session session, Topic topic) {
        System.out.println("\n  Write your tweet: ");
        BufferedReader br = new BufferedReader(new InputStreamReader(System.in));
        String text = "";
        try {
            text = br.readLine();
            if (!text.equals("")) {
                MessageProducer producer = session.createProducer(topic);
                TextMessage textMessage = session.createTextMessage(text);
                producer.send(textMessage);
                System.out.println("  Tweet sent");
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public static void subscribeChannel(Session session, Context context) {
        System.out.println("\n  Write name of the channel to subscribe to: ");
        BufferedReader br = new BufferedReader(new InputStreamReader(System.in));
        try {
            final String name = br.readLine();
            if (topicExists(name) && !subscribedChannelsMap.containsKey(name)) {
                Topic sub = (Topic) context.lookup(name);
                MessageConsumer receiver = session.createConsumer(sub);
                subscribedChannelsMap.put(name, receiver);

                receiver.setMessageListener(new MessageListener() {
                    public void onMessage(Message message) {
                        synchronized (newTweetsCount) {
                            newTweetsCount++;
                        }
                        TextMessage textMessage = (TextMessage) message;
                        try {
                            tweets.addLast(new Tweet(name, textMessage.getText()));
                            //System.out.println("Received message: " + textMessage.getText());
                        } catch (JMSException e) {
                            e.printStackTrace();
                        }
                    }
                });
                System.out.println("  Subscribed to " + name);
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public static void unsubscribeChannel() {
        System.out.println("\n  Write name of the channel to unsubscribe: ");
        BufferedReader br = new BufferedReader(new InputStreamReader(System.in));
        try {
            final String name = br.readLine();
            if (topicExists(name) && subscribedChannelsMap.containsKey(name)) {
                MessageConsumer receiver = subscribedChannelsMap.get(name);
                receiver.setMessageListener(null);
                subscribedChannelsMap.remove(name);
                System.out.println("  Unsubscribed from " + name);
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public static void printAllTweets() {
        synchronized (newTweetsCount) {
            newTweetsCount = 0;
        }
        System.out.println("\n  Recent tweets: ");
        for (Tweet t: tweets) {
            System.out.println(t.getChannel() + " >> " + t.getMessage());
        }
    }

    public static void printAllSubscribedChannels() {
        System.out.println("\n  Subscribed channels: ");
        for (String t: subscribedChannelsMap.keySet()) {
            System.out.println(t);
        }
    }






    public static void main(String[] argv) {
        try {
            BufferedReader br = new BufferedReader(new InputStreamReader(System.in));
            System.out.print("Enter your channel name: ");
            String channelName = br.readLine();

            Context context = new InitialContext();
            ConnectionFactory factory = (ConnectionFactory) context.lookup("ConnectionFactory");
            Connection connection = factory.createConnection();
            Session session = connection.createSession(false, Session.AUTO_ACKNOWLEDGE);
            Topic myTopic = createTopic(channelName, context);

            connection.start();

            System.out.println("Channel " + channelName + " connected");

            boolean working = true;
            while (working) {
                printMenu(myTopic.getTopicName());
                String choice = br.readLine();
                switch (choice) {
                    case "1":
                        writeTweet(session, myTopic);
                        break;
                    case "2":
                        printAllTweets();
                        break;
                    case "3":
                        printAllChannels();
                        break;
                    case "4":
                        printAllSubscribedChannels();
                        break;
                    case "5":
                        subscribeChannel(session, context);
                        break;
                    case "6":
                        unsubscribeChannel();
                        break;
                    case "q":
                        working = false;
                        System.out.println("Exiting...");
                        break;
                    default:
                        System.out.println("Unknown command");
                        break;
                }
                if (working) {
                    System.out.println("Hit [Enter] to get back to main menu");
                    br.readLine();
                }
            }

            context.close();
            connection.close();
        } catch (Exception jmse) {
            System.out.println("Exception occurred : " + jmse.toString());
            jmse.printStackTrace();
        }
    }
}

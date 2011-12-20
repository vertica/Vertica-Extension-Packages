-------- Demonstrate the use of the sms function ----------
-- NOTE: MUST INSTANTIATE THE STATEMENTS BELOW FIRST. --

CREATE TABLE Accounts (id INT, sid VARCHAR, auth VARCHAR, from_num VARCHAR);

-- Obtain a twilio account. Upon login, the associated "account sid" and "auth token" can be obtained at https://www.twilio.com/user/account
-- Place the above two fields into the INSERT statement below. 
-- The "from phone number" field below can be set to the "Call the Sandbox number" also at https://www.twilio.com/user/account

-- See http://www.twilio.com/docs/quickstart/php/connect for additional background on sending an SMS via the twilio web sevice.
INSERT INTO Accounts values (1, '<account sid>', '<auth token>', '<from phone number>');

-- Invoke UDF

-- Send an SMS to a phone number (e.g. '+19781234567'), with content specified in the last field of the function call
SELECT SMS(sid, auth, from_num, '<to phone num>', 'test Msg') FROM Accounts a where a.id = 1;

-- The above function can also be used to send a set of messages to a
-- group of people. To do so, First create a Recipients table, each
-- row storing the phone number of one recipient. Next create a
-- Messages table, each row storing one message. Finally invoke the
-- function with the following signature:  
-- SELECT SMS(a.sid, a.auth, a.from_num, r.phone_num, m.content) FROM Accounts a, Recipients r, Messages m;


-- syntax sugar function to send SMS
CREATE FUNCTION mySMS(content VARCHAR(200)) RETURN VARCHAR(65000)
  AS BEGIN
     RETURN SMS('<account sid>', '<auth token>', '<from phone num>', '<to phone num>', content);
  END;
select mySMS('Another test message');
DROP FUNCTION mySMS(content VARCHAR(200));

DROP TABLE Accounts;

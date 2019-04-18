/*
	Copyright (c) 2017 Robert Bosch GmbH
	All rights reserved.

	This source code is licensed under the MIT license found in the
	LICENSE.md file in the root directory of this source tree.
*/


pragma solidity ^0.4.23;
  
/* owner contract */
contract SecureEdgeDeviceOwner {
    /* Define variable owner of the type address*/
    address owner;
  
    /* sets the owner of the contract to the creator */
    constructor () public {
        owner = msg.sender;
    }
  
    /* function definitions including this modifier
    can only be called by the owner of the contract
    */
    modifier onlyOwner {
        require(msg.sender == owner,
            "Only owner can call this function."
        );
        _;
    }
      
    /* can be used to withdraw all contract funds - currently disabled */
    /*
    function withdraw() public onlyOwner {
        owner.transfer(address(this).balance);
    }
    */
      
    /* Function to recover the funds of the contract - should always be implemented */
    /* sends all remaining contract funds back to owner - Owner can collect funds of negative votes with that function!!! */
    function kill() public onlyOwner {
        selfdestruct(owner);
    }
      
    /* reject ether if it does not call a function */
    /* if someone sends ether to a non payable or non existing function then it gets rejected by this default function */
    function() payable public {
        revert("Function not supported");
    }
}
  
/* working contract - derived contract from SecureEdgeDeviceOwner contract */
contract SecureEdgeDevice is SecureEdgeDeviceOwner {
    /* define dynamic variables publicKey and dataHash of type bytes */
    bytes publicKey;
    bytes dataHash;
    /* variable to hold address of consumer */
    address publicKeySenderAccountAddress;
      
    /* counter variables to hold votes for evaluation system */
    uint256 positiveConsumerVotes = 0;
    uint256 totalConsumerVotes = 0;
      
    /* map an address to an integer value to store address and related balance */
    mapping (address => uint256) balanceOf;
  
    /* current price is fixed but could be implemented so that the owner can change the price*/
    uint256 constant currentPrice = 2 ether;
    uint256 constant currentVotingPrice = currentPrice/2;
  
    /* write public key - function payable because we require currentPrice to send along this function call */
    function WritePublicKey(bytes publicKeyArg) public payable {
        /* Check if the sender has provided currentPrice ether along with the message */
        require(msg.value == currentPrice, "Payed price does not match the current data price");
          
        /* store locally who has deposited ether */
        balanceOf[msg.sender] += (msg.value/2);
        balanceOf[owner] += (msg.value/2);
          
        /* store sender information */
        publicKeySenderAccountAddress = msg.sender;
        publicKey = publicKeyArg;
    }
      
    /* read public key and sender address - only the contract owner */
    function ReadPublicKey() public view onlyOwner returns (bytes, address) {
        return (publicKey, publicKeySenderAccountAddress);
    }
      
    /* write data hash - only the contract owner */
    function WriteDataHash(bytes dataHashArg) public onlyOwner {
        dataHash = dataHashArg;
    }
      
    /* read data hash - everyone can read current data hash for free */
    function ReadDataHash() public view returns (bytes) {
        return dataHash;
    }
  
    /* voting function for consumers to vote - for a positive vote, consumer gets refunded currentPrice/2 */
    function rateProducer(bool positive) public {
        /* only authenticated consumers who paid for public key are allowed to vote */
        require(balanceOf[msg.sender] >= currentVotingPrice, "Consumer already voted, no votes left");
        /* producer is not allowed to vote - otherwise owner could create positive votes for himself */
        require(msg.sender != owner, "Owner is not allowed to vote");
          
        /* refund half of the price for positive voting */
        if( true == positive ) {
            /* increment total voting counter */
            totalConsumerVotes += 1;
            /* increment consumer vote counter */
            positiveConsumerVotes += 1;
            /* transfer revenue for positive vote */
            balanceOf[msg.sender] -= currentVotingPrice;
            msg.sender.transfer(currentVotingPrice);
              
            /* transfer the income to owner - otherwise it is stored in the contract itself */
            if(balanceOf[owner] >= currentVotingPrice) {
                balanceOf[owner] -= currentVotingPrice;
                owner.transfer(currentVotingPrice);
            }
        /* don't refund for negative vote */
        } else if ( false == positive ) {
            /* increment total voting counter */
            totalConsumerVotes += 1;
              
            /* keep funds stored in contract - can be distributed somehow */
            balanceOf[msg.sender] -= currentVotingPrice;
            balanceOf[owner] -= currentVotingPrice;
        } else {
            revert("Parameter value not supported");
        }
    }
      
    /* read producer rating - consumer could decide based on this rating if he wants to work with producer */
    function readProducerRating() public view returns (uint256) {
        return (positiveConsumerVotes*100/totalConsumerVotes);
    }
      
    /* client could get back his funds if he doesn't want to read data hash */
    /*
    function RefundConsumer () public {
        require(balanceOf[msg.sender] >= 0);
        msg.sender.transfer(balanceOf[msg.sender]);
        balanceOf[msg.sender] = 0;
    }
    */
}
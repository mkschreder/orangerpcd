global.WebSocket = require("ws"); 
global.$ = require("jquery-deferred"); 
require("./rpc2"); 
require("./sha1.js"); 

RPC.$connect("ws://localhost:1234").done(function(){
	RPC.$login("admin","admin").done(function(result){
		console.log("login: "+JSON.stringify(result)); 
		RPC.$list("*").done(function(result){
			console.log("List: "+JSON.stringify(result)); 
		});
		RPC.$call("/simple", "largejson", {}).done(function(result){
			console.log("received json "+String(JSON.stringify(result)).length); 
		}); 
		RPC.$call("/session", "test", {}).done(function(result){
			RPC.$call("/session", "test", {}).done(function(result){
				console.log("Done!"); 
			}); 
		}); 
		RPC.$call("/session", "access", {
			scope: "page", 
			object: "juci-test",
			method: "car",
			perms: "w"
		}).done(function(result){
			console.log("Session.access: "+JSON.stringify(result)); 
		}); 
	}).fail(function(){
		console.log("Login failed!"); 
	}); 
	/*RPC.$call("/session", "access", {}).done(function(result){
		console.log("session.access: "+result); 
	}); 
	RPC.$call("/uci", "configs", {}).done(function(result){
		console.log("configs: "+result); 
	}); 
	RPC.$call("/uci", "revert", {}).done(function(result){
		console.log("revert: "+result); 
	}); 
	RPC.$call("/juci/ethernet", "adapters", {foo:"bar"}).done(function(result){
		console.log("Netstat: "+result); 
	}); 
	RPC.$call("/simple","print_hello", { message: "Hello World!" }).done(function(result){
		console.log("Call completed: "+result); 
	}); 
	RPC.$call("/simple","print_hello", { message: "Hello World!" }).done(function(result){
		console.log("Call completed! "+result); 
	}); */
}); 


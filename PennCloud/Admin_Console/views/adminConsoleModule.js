angular.module('adminConsole', ['ngRoute']).config(function($routeProvider) {
	$routeProvider.when('/', {controller:ServerCtrl, templateUrl:'home.html'});
	$routeProvider.when('/data', {controller:DataCtrl, templateUrl:'data.html'});
});


function ServerCtrl($scope, $http, $location, $interval) {
	
	$scope.goTo = function(path, address) {
		if (address) {
			$location.path(path).search({address: address});
		}
		else {
			$location.path(path);
		}
	};
	
	$scope.killServer = function(address) {
		$http.post('kill', address).then(function() {
			console.log('Successfully sent kill command');
		},
		function() {
			console.log('Error sending kill command');
		})
	};
	
	var poll;
	var getData = function() {
		$http.get('feServerData').then(function (response) {
			$scope.feServers = response.data;
		},
		function() {
			console.log('Error retrieving feServerData');
		});
		
		$http.get('beServerData').then(function (response) {
			$scope.beServers = response.data;
		},
		function() {
			console.log('Error retrieving beServerData');
		});
	}
	
	$scope.$on('$destroy', function() {
		console.log('Cancelling server status polling');
		$interval.cancel(poll);
	});

	getData();
	// Refresh data every 10 seconds
	poll = $interval(getData, 10000);
}

function DataCtrl($scope, $http, $location) {
	$scope.goTo = function(path) {
		$location.path(path);
	};
	
	var address = $location.search().address ? $location.search().address : '';
	$http.get('bigTableData/' + address).then(function (response) {
		$scope.bigTableData = response.data;
		
//		for (var userName in $scope.bigTableData) {
//		    if ($scope.bigTableData.hasOwnProperty(userName)) {
//		    	$scope.bigTableData[userName].columns.sort(function(a, b){
//		    	    if(a.filename < b.filename) return -1;
//		    	    if(a.filename > b.filename) return 1;
//		    	    return 0;
//		    	});
//		    }
//		}
	},
	function() {
		$scope.bigTableData = [];
		console.log('Error retrieving feServerData');
	});
	
	$scope.countColumns = function (columns) {
		return Object.keys(columns).length;
	};
	
	$scope.getMetadata = function (metadata) {
		var copy = angular.copy(metadata);
		delete copy.content;
		return copy;
	}
}
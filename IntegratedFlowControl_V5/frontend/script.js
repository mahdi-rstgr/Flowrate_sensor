/**
 * Integrated Flow Control System - Main JavaScript
 * Manages unified interface for flow sensors and pump control
 */

class IntegratedFlowController {
    constructor() {
        this.charts = {};
        this.updateInterval = null;
        this.sensorData = {};
        this.pumpStatus = {};
        this.isRecording = false;
        this.tableRowCount = 0;
        this.maxTableRows = 30;
        
        this.init();
    }
    
    init() {
        this.setupEventListeners();
        this.setupCharts();
        this.startDataUpdates();
        this.updateSystemStatus();
        
        console.log('Integrated Flow Controller initialized');
    }
    
    // =================================================================================
    // Event Listeners Setup
    // =================================================================================
    
    setupEventListeners() {
        // Tab navigation
        document.querySelectorAll('.tab-button').forEach(button => {
            button.addEventListener('click', (e) => {
                this.switchTab(e.target.getAttribute('data-tab'));
            });
        });
        
        // Flow sensor controls
        this.setupSensorEventListeners();
        
        // Pump controls
        this.setupPumpEventListeners();
        
        // Data management
        this.setupDataEventListeners();
    }
    
    setupSensorEventListeners() {
        // Recording controls
        document.getElementById('startRecording')?.addEventListener('click', () => this.startRecording());
        document.getElementById('stopRecording')?.addEventListener('click', () => this.stopRecording());
        document.getElementById('startRecording2')?.addEventListener('click', () => this.startRecording());
        document.getElementById('stopRecording2')?.addEventListener('click', () => this.stopRecording());
        
        // Sensor toggles
        ['sensor1Toggle', 'sensor2Toggle', 'sensor3Toggle', 'sensor4Toggle'].forEach(id => {
            const toggle = document.getElementById(id);
            if (toggle) {
                toggle.addEventListener('change', (e) => {
                    const sensorId = parseInt(id.replace('sensorToggle', '').replace('sensor', ''));
                    this.toggleSensor(sensorId, e.target.checked);
                });
            }
        });
        
        // Download controls
        document.getElementById('downloadData')?.addEventListener('click', () => this.downloadSensorData());
    }
    
    setupPumpEventListeners() {
        // Emergency stop
        document.getElementById('emergencyStop')?.addEventListener('click', () => this.emergencyStop());
        document.getElementById('emergencyStop2')?.addEventListener('click', () => this.emergencyStop());
        
        // Individual pump controls
        document.querySelectorAll('.pump-start').forEach(btn => {
            btn.addEventListener('click', (e) => {
                const pumpId = e.target.getAttribute('data-pump');
                this.startPump(pumpId);
            });
        });
        
        document.querySelectorAll('.pump-stop').forEach(btn => {
            btn.addEventListener('click', (e) => {
                const pumpId = e.target.getAttribute('data-pump');
                this.stopPump(pumpId);
            });
        });
        
        // Quick start buttons (overview tab)
        ['quickStart1', 'quickStart2', 'quickStart3', 'quickStart4'].forEach(id => {
            document.getElementById(id)?.addEventListener('click', () => {
                const pumpId = id.replace('quickStart', '');
                this.quickStartPump(pumpId);
            });
        });
        
        // Multi-pump controls
        document.getElementById('selectAllPumps')?.addEventListener('change', (e) => {
            this.selectAllPumps(e.target.checked);
        });
        
        document.getElementById('startSelectedPumps')?.addEventListener('click', () => {
            this.startSelectedPumps();
        });
        
        // Pump selection checkboxes
        document.querySelectorAll('.pump-checkbox').forEach(checkbox => {
            checkbox.addEventListener('change', () => {
                this.updateSelectedPumpCount();
            });
        });
    }
    
    setupDataEventListeners() {
        document.getElementById('downloadCsv')?.addEventListener('click', () => this.downloadSensorData());
        document.getElementById('exportSystemReport')?.addEventListener('click', () => this.exportSystemReport());
        
        // Storage management
        document.getElementById('cleanStorage')?.addEventListener('click', () => this.cleanStorage());
        document.getElementById('refreshStorage')?.addEventListener('click', () => this.refreshStorageStatus());
    }
    
    // =================================================================================
    // Tab Management
    // =================================================================================
    
    switchTab(tabName) {
        // Update tab buttons
        document.querySelectorAll('.tab-button').forEach(btn => {
            btn.classList.remove('active');
        });
        document.querySelector(`[data-tab="${tabName}"]`).classList.add('active');
        
        // Update tab content
        document.querySelectorAll('.tab-panel').forEach(panel => {
            panel.classList.remove('active');
        });
        document.getElementById(tabName).classList.add('active');
        
        // Refresh charts when switching to chart tabs
        if (tabName === 'overview' || tabName === 'sensors') {
            setTimeout(() => {
                Object.values(this.charts).forEach(chart => {
                    if (chart) chart.resize();
                });
            }, 100);
        }
        
        // Refresh storage status when switching to data management tab
        if (tabName === 'data') {
            setTimeout(() => {
                this.refreshStorageStatus();
            }, 100);
        }
    }
    
    // =================================================================================
    // Chart Setup
    // =================================================================================
    
    setupCharts() {
        this.setupOverviewChart();
        this.setupDetailedFlowChart();
        this.setupIndividualSensorCharts();
    }
    
    setupOverviewChart() {
        const ctx = document.getElementById('overviewChart');
        if (!ctx) return;
        
        this.charts.overview = new Chart(ctx, {
            type: 'line',
            data: {
                labels: [],
                datasets: [
                    {
                        label: 'Sensor 1',
                        data: [],
                        borderColor: '#007bff',
                        backgroundColor: 'rgba(0, 123, 255, 0.1)',
                        tension: 0.4
                    },
                    {
                        label: 'Sensor 2',
                        data: [],
                        borderColor: '#28a745',
                        backgroundColor: 'rgba(40, 167, 69, 0.1)',
                        tension: 0.4
                    },
                    {
                        label: 'Sensor 3',
                        data: [],
                        borderColor: '#ffc107',
                        backgroundColor: 'rgba(255, 193, 7, 0.1)',
                        tension: 0.4
                    },
                    {
                        label: 'Sensor 4',
                        data: [],
                        borderColor: '#dc3545',
                        backgroundColor: 'rgba(220, 53, 69, 0.1)',
                        tension: 0.4
                    }
                ]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                scales: {
                    y: {
                        beginAtZero: true,
                        title: {
                            display: true,
                            text: 'Flow Rate (mL/min)'
                        }
                    },
                    x: {
                        title: {
                            display: true,
                            text: 'Time'
                        }
                    }
                },
                plugins: {
                    title: {
                        display: true,
                        text: 'Real-time Flow Monitoring'
                    },
                    legend: {
                        display: true
                    },
                    tooltip: {
                        callbacks: {
                            label: function(context) {
                                let label = context.dataset.label || '';
                                if (label) {
                                    label += ': ';
                                }
                                if (context.parsed.y !== null) {
                                    label += context.parsed.y.toFixed(3) + ' mL/min';
                                }
                                return label;
                            }
                        }
                    }
                },
                animation: {
                    duration: 0
                }
            }
        });
    }
    
    setupDetailedFlowChart() {
        const ctx = document.getElementById('flowChart');
        if (!ctx) return;
        
        this.charts.detailed = new Chart(ctx, {
            type: 'line',
            data: {
                labels: [],
                datasets: [
                    {
                        label: 'Sensor 1 Flow',
                        data: [],
                        borderColor: '#007bff',
                        backgroundColor: 'rgba(0, 123, 255, 0.1)',
                        tension: 0.2
                    },
                    {
                        label: 'Sensor 2 Flow',
                        data: [],
                        borderColor: '#28a745',
                        backgroundColor: 'rgba(40, 167, 69, 0.1)',
                        tension: 0.2
                    },
                    {
                        label: 'Sensor 3 Flow',
                        data: [],
                        borderColor: '#ffc107',
                        backgroundColor: 'rgba(255, 193, 7, 0.1)',
                        tension: 0.2
                    },
                    {
                        label: 'Sensor 4 Flow',
                        data: [],
                        borderColor: '#dc3545',
                        backgroundColor: 'rgba(220, 53, 69, 0.1)',
                        tension: 0.2
                    }
                ]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                scales: {
                    y: {
                        beginAtZero: true,
                        title: {
                            display: true,
                            text: 'Flow Rate (mL/min)'
                        }
                    },
                    x: {
                        title: {
                            display: true,
                            text: 'Sample Number'
                        }
                    }
                },
                plugins: {
                    title: {
                        display: true,
                        text: 'Detailed Flow Analysis'
                    },
                    tooltip: {
                        callbacks: {
                            label: function(context) {
                                let label = context.dataset.label || '';
                                if (label) {
                                    label += ': ';
                                }
                                if (context.parsed.y !== null) {
                                    label += context.parsed.y.toFixed(3) + ' mL/min';
                                }
                                return label;
                            }
                        }
                    }
                },
                animation: {
                    duration: 0
                }
            }
        });
    }
    
    setupIndividualSensorCharts() {
        const sensorColors = ['#007bff', '#28a745', '#ffc107', '#dc3545'];
        const sensorNames = ['Sensor 1', 'Sensor 2', 'Sensor 3', 'Sensor 4'];
        
        for (let i = 1; i <= 4; i++) {
            const ctx = document.getElementById(`sensor${i}Chart`);
            if (!ctx) continue;
            
            this.charts[`sensor${i}`] = new Chart(ctx, {
                type: 'line',
                data: {
                    labels: [],
                    datasets: [{
                        label: `${sensorNames[i-1]} Flow`,
                        data: [],
                        borderColor: sensorColors[i-1],
                        backgroundColor: sensorColors[i-1] + '20',
                        tension: 0.3,
                        fill: true
                    }]
                },
                options: {
                    responsive: true,
                    maintainAspectRatio: false,
                    scales: {
                        y: {
                            beginAtZero: true,
                            title: {
                                display: true,
                                text: 'Flow Rate (mL/min)'
                            }
                        },
                        x: {
                            title: {
                                display: true,
                                text: 'Time'
                            }
                        }
                    },
                    plugins: {
                        title: {
                            display: true,
                            text: `${sensorNames[i-1]} Flow Rate`
                        },
                        legend: {
                            display: false
                        },
                        tooltip: {
                            callbacks: {
                                label: function(context) {
                                    let label = context.dataset.label || '';
                                    if (label) {
                                        label += ': ';
                                    }
                                    if (context.parsed.y !== null) {
                                        label += context.parsed.y.toFixed(3) + ' mL/min';
                                    }
                                    return label;
                                }
                            }
                        }
                    },
                    animation: {
                        duration: 0
                    }
                }
            });
        }
    }
    
    // =================================================================================
    // Data Updates
    // =================================================================================
    
    startDataUpdates() {
        // Update every 1 second
        this.updateInterval = setInterval(() => {
            this.updateAllData();
        }, 1000);
        
        // Initial update
        this.updateAllData();
    }
    
    async updateAllData() {
        try {
            const updates = [
                this.updateSensorData(),
                this.updatePumpData(),
                this.updateSystemStatus()
            ];
            
            // Update storage status if data management tab is active
            const activeTab = document.querySelector('.tab-panel.active');
            if (activeTab && activeTab.id === 'data') {
                updates.push(this.refreshStorageStatus());
            }
            
            await Promise.all(updates);
            this.updateLastUpdateTime();
        } catch (error) {
            console.error('Error updating data:', error);
        }
    }
    
    async updateSensorData() {
        try {
            const response = await fetch('/api/flow-sensors');
            const result = await response.json();
            
            if (result.success && result.data) {
                this.sensorData = result.data;
                this.updateSensorUI();
                this.updateCharts();
                this.updateSensorTable();
                this.updateRecordingStatus();
            }
        } catch (error) {
            console.error('Error updating sensor data:', error);
            this.updateConnectionStatus('esp8266', false, 'Connection error');
        }
    }
    
    async updatePumpData() {
        try {
            const response = await fetch('/api/pumps/status');
            const result = await response.json();
            
            if (result.success && result.status) {
                this.pumpStatus = result.status;
                this.updatePumpUI();
            }
        } catch (error) {
            console.error('Error updating pump data:', error);
            this.updateConnectionStatus('arduino', false, 'Connection error');
        }
    }
    
    async updateSystemStatus() {
        try {
            const response = await fetch('/api/system/status');
            const result = await response.json();
            
            if (result.success) {
                this.updateConnectionStatus('arduino', result.arduino.connected, result.arduino.port);
                this.updateConnectionStatus('esp8266', result.esp8266.connected, result.esp8266.ip);
            }
        } catch (error) {
            console.error('Error updating system status:', error);
        }
    }
    
    // =================================================================================
    // UI Updates
    // =================================================================================
    
    updateSensorUI() {
        if (!this.sensorData) return;
        
        // Update overview sensor cards
        Object.keys(this.sensorData).forEach(key => {
            if (key.startsWith('s')) {
                const sensorNum = key.replace('s', '');
                const sensor = this.sensorData[key];
                
                this.updateElement(`sensor${sensorNum}Flow`, 
                    sensor.flow_1s ? sensor.flow_1s.toFixed(3) : '--');
                    
                const statusElement = document.getElementById(`sensor${sensorNum}Status`);
                if (statusElement) {
                    statusElement.textContent = sensor.ok ? 'OK' : 'Error';
                    statusElement.className = `sensor-status ${sensor.ok ? '' : 'error'}${!sensor.enabled ? ' disabled' : ''}`;
                }
            }
        });
    }
    
    updatePumpUI() {
        if (!this.pumpStatus) return;
        
        let activePumps = 0;
        
        Object.keys(this.pumpStatus).forEach(pumpId => {
            const isRunning = this.pumpStatus[pumpId] === 1;
            if (isRunning) activePumps++;
            
            const statusText = isRunning ? 'Running' : 'Stopped';
            
            // Update overview cards
            this.updateElement(`pump${pumpId}Status`, statusText);
            
            // Update detailed cards  
            this.updateElement(`detailedPump${pumpId}Status`, statusText);
            
            // Update status indicators
            document.querySelectorAll(`[id*="pump${pumpId}Status"]`).forEach(el => {
                el.className = `pump-status-indicator ${isRunning ? 'running' : ''}`;
            });
        });
        
        // Update active pump count
        this.updateElement('activePumpCount', activePumps);
    }
    
    updateRecordingStatus() {
        if (this.sensorData && this.sensorData.run) {
            this.isRecording = this.sensorData.run.recording;
            const statusText = this.isRecording ? 'Recording' : 'Not Recording';
            
            ['recordingStatus', 'recordingStatus2'].forEach(id => {
                const element = document.getElementById(id);
                if (element) {
                    element.textContent = statusText;
                    element.className = `recording-status ${this.isRecording ? 'recording' : ''}`;
                }
            });
        }
    }
    
    updateCharts() {
        if (!this.sensorData) return;
        
        const now = new Date().toLocaleTimeString();
        
        // Update overview chart
        if (this.charts.overview) {
            const chart = this.charts.overview;
            
            // Add new data point
            chart.data.labels.push(now);
            
            ['s1', 's2', 's3', 's4'].forEach((key, index) => {
                const sensor = this.sensorData[key];
                const flow = sensor && sensor.enabled && sensor.ok ? sensor.flow_1s : 0;
                chart.data.datasets[index].data.push(flow);
            });
            
            // Keep only last (was 20) 100 points
            if (chart.data.labels.length > 100) {
                chart.data.labels.shift();
                chart.data.datasets.forEach(dataset => {
                    dataset.data.shift();
                });
            }
            
            chart.update();
        }
        
        // Update detailed chart  
        if (this.charts.detailed) {
            const chart = this.charts.detailed;
            
            chart.data.labels.push(this.tableRowCount);
            
            ['s1', 's2', 's3', 's4'].forEach((key, index) => {
                const sensor = this.sensorData[key];
                const flow = sensor && sensor.enabled && sensor.ok ? sensor.flow_1s : 0;
                chart.data.datasets[index].data.push(flow);
            });
            
            // Keep only last (was 50) 250 points
            if (chart.data.labels.length > 250) {
                chart.data.labels.shift();
                chart.data.datasets.forEach(dataset => {
                    dataset.data.shift();
                });
            }
            
            chart.update();
        }
        
        // Update individual sensor charts
        for (let i = 1; i <= 4; i++) {
            const chartKey = `sensor${i}`;
            const sensorKey = `s${i}`;
            
            if (this.charts[chartKey]) {
                const chart = this.charts[chartKey];
                const sensor = this.sensorData[sensorKey];
                const flow = sensor && sensor.enabled && sensor.ok ? sensor.flow_1s : 0;
                
                // Add new data point
                chart.data.labels.push(now);
                chart.data.datasets[0].data.push(flow);
                
                // Keep only last (was 30) 150 points
                if (chart.data.labels.length > 150) {
                    chart.data.labels.shift();
                    chart.data.datasets[0].data.shift();
                }
                
                chart.update();
            }
        }
    } 
    
    updateSensorTable() {
        if (!this.sensorData) return;
        
        const tbody = document.querySelector('#sensorDataTable tbody');
        if (!tbody) return;
        
        const now = new Date().toLocaleTimeString();
        const row = tbody.insertRow(0);
        
        row.innerHTML = `
            <td>${now}</td>
            <td>${this.getSensorValue('s1', 'flow_1s')}</td>
            <td>${this.getSensorValue('s1', 'temp_1s')}</td>
            <td>${this.getSensorValue('s2', 'flow_1s')}</td>
            <td>${this.getSensorValue('s2', 'temp_1s')}</td>
            <td>${this.getSensorValue('s3', 'flow_1s')}</td>
            <td>${this.getSensorValue('s3', 'temp_1s')}</td>
            <td>${this.getSensorValue('s4', 'flow_1s')}</td>
            <td>${this.getSensorValue('s4', 'temp_1s')}</td>
        `;
        
        // Remove old rows
        while (tbody.rows.length > this.maxTableRows) {
            tbody.deleteRow(-1);
        }
        
        this.tableRowCount++;
    }
    
    updateConnectionStatus(device, connected, info) {
        const statusDot = document.getElementById(`${device}Status`);
        const statusText = document.getElementById(`${device}StatusText`);
        
        if (statusDot) {
            statusDot.className = `status-dot ${connected ? 'connected' : 'disconnected'}`;
        }
        
        if (statusText) {
            if (connected) {
                statusText.textContent = `Connected (${info})`;
            } else {
                statusText.textContent = info || 'Disconnected';
            }
        }
    }
    
    updateLastUpdateTime() {
        const now = new Date().toLocaleTimeString();
        this.updateElement('lastUpdate', now);
    }
    
    // =================================================================================
    // Sensor Control Methods
    // =================================================================================
    
    async startRecording() {
        try {
            const response = await fetch('/api/sensors/start-recording', { method: 'POST' });
            const result = await response.json();
            
            if (result.success) {
                this.showNotification('Recording started successfully', 'success');
            } else {
                this.showNotification(`Failed to start recording: ${result.message}`, 'error');
            }
        } catch (error) {
            console.error('Error starting recording:', error);
            this.showNotification('Error starting recording', 'error');
        }
    }
    
    async stopRecording() {
        try {
            const response = await fetch('/api/sensors/stop-recording', { method: 'POST' });
            const result = await response.json();
            
            if (result.success) {
                this.showNotification('Recording stopped successfully', 'success');
            } else {
                this.showNotification(`Failed to stop recording: ${result.message}`, 'error');
            }
        } catch (error) {
            console.error('Error stopping recording:', error);
            this.showNotification('Error stopping recording', 'error');
        }
    }
    
    async toggleSensor(sensorId, enabled) {
        try {
            const response = await fetch(`/api/sensors/toggle/${sensorId}`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ enabled })
            });
            
            const result = await response.json();
            
            if (result.success) {
                this.showNotification(`Sensor ${sensorId} ${enabled ? 'enabled' : 'disabled'}`, 'success');
            } else {
                this.showNotification(`Failed to toggle sensor: ${result.message}`, 'error');
            }
        } catch (error) {
            console.error('Error toggling sensor:', error);
            this.showNotification('Error toggling sensor', 'error');
        }
    }
    
    async downloadSensorData() {
        try {
            const response = await fetch('/api/sensors/download-csv');
            
            if (response.ok) {
                const blob = await response.blob();
                const url = window.URL.createObjectURL(blob);
                const a = document.createElement('a');
                a.href = url;
                a.download = `flow_data_${new Date().getTime()}.csv`;
                document.body.appendChild(a);
                a.click();
                document.body.removeChild(a);
                window.URL.revokeObjectURL(url);
                
                this.showNotification('CSV downloaded successfully', 'success');
            } else {
                this.showNotification('No data available for download', 'warning');
            }
        } catch (error) {
            console.error('Error downloading CSV:', error);
            this.showNotification('Error downloading data', 'error');
        }
    }
    
    // =================================================================================
    // Pump Control Methods
    // =================================================================================
    
    async startPump(pumpId) {
        const config = this.getPumpConfig(pumpId);
        
        try {
            const response = await fetch('/api/pumps/start', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({
                    pump: pumpId,
                    rpm: config.rpm,
                    time: config.time,
                    continuous: config.continuous ? 1 : 0
                })
            });
            
            const result = await response.json();
            
            if (result.success) {
                this.showNotification(`Pump ${pumpId} started at ${config.rpm} RPM`, 'success');
            } else {
                this.showNotification(`Failed to start pump: ${result.message}`, 'error');
            }
        } catch (error) {
            console.error('Error starting pump:', error);
            this.showNotification('Error starting pump', 'error');
        }
    }
    
    async stopPump(pumpId) {
        try {
            const response = await fetch('/api/pumps/stop', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ pump: pumpId })
            });
            
            const result = await response.json();
            
            if (result.success) {
                this.showNotification(`Pump ${pumpId} stopped`, 'success');
            } else {
                this.showNotification(`Failed to stop pump: ${result.message}`, 'error');
            }
        } catch (error) {
            console.error('Error stopping pump:', error);
            this.showNotification('Error stopping pump', 'error');
        }
    }
    
    quickStartPump(pumpId) {
        // Quick start with default settings (100 RPM, 10 seconds)
        const tempConfig = { rpm: 100, time: 10, continuous: false };
        
        fetch('/api/pumps/start', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({
                pump: pumpId,
                rpm: tempConfig.rpm,
                time: tempConfig.time,
                continuous: 0
            })
        })
        .then(response => response.json())
        .then(result => {
            if (result.success) {
                this.showNotification(`Pump ${pumpId} quick started`, 'success');
            } else {
                this.showNotification(`Failed to quick start: ${result.message}`, 'error');
            }
        })
        .catch(error => {
            console.error('Error quick starting pump:', error);
            this.showNotification('Error quick starting pump', 'error');
        });
    }
    
    async emergencyStop() {
        try {
            const response = await fetch('/api/pumps/emergency-stop', { method: 'POST' });
            const result = await response.json();
            
            if (result.success) {
                this.showNotification('EMERGENCY STOP ACTIVATED - All pumps stopped', 'warning');
            } else {
                this.showNotification(`Emergency stop failed: ${result.message}`, 'error');
            }
        } catch (error) {
            console.error('Error executing emergency stop:', error);
            this.showNotification('Error executing emergency stop', 'error');
        }
    }
    
    selectAllPumps(checked) {
        document.querySelectorAll('.pump-checkbox').forEach(checkbox => {
            checkbox.checked = checked;
        });
        this.updateSelectedPumpCount();
    }
    
    startSelectedPumps() {
        const selectedPumps = Array.from(document.querySelectorAll('.pump-checkbox:checked'))
            .map(cb => cb.getAttribute('data-pump'));
        
        if (selectedPumps.length === 0) {
            this.showNotification('No pumps selected', 'warning');
            return;
        }
        
        const batchRpm = parseFloat(document.getElementById('batchRpm').value);
        const batchTime = parseInt(document.getElementById('batchTime').value);
        const batchContinuous = document.getElementById('batchContinuous').checked;
        
        selectedPumps.forEach(pumpId => {
            fetch('/api/pumps/start', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({
                    pump: pumpId,
                    rpm: batchRpm,
                    time: batchTime,
                    continuous: batchContinuous ? 1 : 0
                })
            })
            .then(response => response.json())
            .then(result => {
                if (!result.success) {
                    console.error(`Failed to start pump ${pumpId}: ${result.message}`);
                }
            });
        });
        
        this.showNotification(`Started ${selectedPumps.length} selected pump(s)`, 'success');
    }
    
    updateSelectedPumpCount() {
        const count = document.querySelectorAll('.pump-checkbox:checked').length;
        const elements = document.querySelectorAll('#selectedPumps');
        elements.forEach(el => el.textContent = count);
    }
    
    // =================================================================================
    // Data Management
    // =================================================================================
    
    async exportSystemReport() {
        try {
            const systemData = {
                timestamp: new Date().toISOString(),
                sensors: this.sensorData,
                pumps: this.pumpStatus,
                recording: this.isRecording
            };
            
            const blob = new Blob([JSON.stringify(systemData, null, 2)], 
                { type: 'application/json' });
            const url = window.URL.createObjectURL(blob);
            const a = document.createElement('a');
            a.href = url;
            a.download = `system_report_${new Date().getTime()}.json`;
            document.body.appendChild(a);
            a.click();
            document.body.removeChild(a);
            window.URL.revokeObjectURL(url);
            
            this.showNotification('System report exported', 'success');
        } catch (error) {
            console.error('Error exporting system report:', error);
            this.showNotification('Error exporting report', 'error');
        }
    }
    
    // =================================================================================
    // Helper Methods
    // =================================================================================
    
    getPumpConfig(pumpId) {
        const rpmInput = document.querySelector(`input.rpm-input[data-pump="${pumpId}"]`);
        const timeInput = document.querySelector(`input.time-input[data-pump="${pumpId}"]`);
        const continuousInput = document.querySelector(`input.continuous-checkbox[data-pump="${pumpId}"]`);
        
        return {
            rpm: rpmInput ? parseFloat(rpmInput.value) : 100,
            time: timeInput ? parseInt(timeInput.value) : 10,
            continuous: continuousInput ? continuousInput.checked : false
        };
    }
    
    getSensorValue(sensorKey, valueType) {
        if (this.sensorData && this.sensorData[sensorKey] && 
            this.sensorData[sensorKey][valueType] !== undefined) {
            return this.sensorData[sensorKey][valueType].toFixed(3);
        }
        return '--';
    }
    
    updateElement(id, value) {
        const element = document.getElementById(id);
        if (element) element.textContent = value;
    }
    
    showNotification(message, type = 'info') {
        // Create notification element if it doesn't exist
        let container = document.getElementById('notification-container');
        if (!container) {
            container = document.createElement('div');
            container.id = 'notification-container';
            container.style.cssText = `
                position: fixed;
                top: 20px;
                right: 20px;
                z-index: 10000;
                display: flex;
                flex-direction: column;
                gap: 10px;
            `;
            document.body.appendChild(container);
        }
        
        const notification = document.createElement('div');
        notification.style.cssText = `
            padding: 12px 20px;
            border-radius: 6px;
            color: white;
            font-weight: 500;
            max-width: 400px;
            box-shadow: 0 4px 6px rgba(0, 0, 0, 0.15);
            transform: translateX(100%);
            transition: transform 0.3s ease;
        `;
        
        // Set background color based on type
        const colors = {
            success: '#28a745',
            error: '#dc3545', 
            warning: '#ffc107',
            info: '#17a2b8'
        };
        notification.style.backgroundColor = colors[type] || colors.info;
        notification.textContent = message;
        
        container.appendChild(notification);
        
        // Animate in
        setTimeout(() => {
            notification.style.transform = 'translateX(0)';
        }, 100);
        
        // Remove after 4 seconds
        setTimeout(() => {
            notification.style.transform = 'translateX(100%)';
            setTimeout(() => {
                if (container.contains(notification)) {
                    container.removeChild(notification);
                }
            }, 300);
        }, 4000);
        
        console.log(`[${type.toUpperCase()}] ${message}`);
    }

    // =================================================================================
    // Storage Management
    // =================================================================================
    
    async refreshStorageStatus() {
        try {
            console.log('Refreshing storage status...');
            const response = await fetch('/api/esp8266/storage');
            
            if (!response.ok) {
                throw new Error(`HTTP ${response.status}: ${response.statusText}`);
            }
            
            const data = await response.json();
            this.updateStorageDisplay(data);
            this.showNotification('Storage status refreshed', 'success');
            
        } catch (error) {
            console.error('Error getting storage status:', error);
            this.showNotification('Failed to get storage status', 'error');
            this.updateStorageDisplay(null);
        }
    }
    
    async cleanStorage() {
        if (!confirm('⚠️ This will delete ALL recorded data files from ESP8266 storage.\\n\\nThis action cannot be undone. Continue?')) {
            return;
        }
        
        if (!confirm('🔔 FINAL CONFIRMATION: This will permanently delete all flow sensor data.\\n\\nHave you downloaded any data you want to keep?')) {
            return;
        }
        
        try {
            console.log('Cleaning ESP8266 storage...');
            
            const button = document.getElementById('cleanStorage');
            if (button) {
                button.disabled = true;
                button.textContent = '🗑️ Cleaning...';
            }
            
            const response = await fetch('/api/esp8266/storage/clean', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                }
            });
            
            const result = await response.text();
            
            if (!response.ok) {
                throw new Error(`HTTP ${response.status}: ${result}`);
            }
            
            this.showNotification('✅ ESP8266 storage cleaned successfully!', 'success');
            
            // Refresh storage status after cleanup
            setTimeout(() => {
                this.refreshStorageStatus();
            }, 1000);
            
        } catch (error) {
            console.error('Error cleaning storage:', error);
            this.showNotification(`Failed to clean storage: ${error.message}`, 'error');
        } finally {
            const button = document.getElementById('cleanStorage');
            if (button) {
                button.disabled = false;
                button.textContent = '🗑️ Clean ESP8266 Storage';
            }
        }
    }
    
    updateStorageDisplay(storageData) {
        if (!storageData) {
            // Update with error state
            document.getElementById('storageUsed').textContent = '--';
            document.getElementById('storageAvailable').textContent = '--';
            document.getElementById('storageTotal').textContent = '--';
            document.getElementById('storageStatus').textContent = 'Unavailable';
            document.getElementById('storageProgressFill').style.width = '0%';
            return;
        }
        
        const { used_kb, total_kb, percent_used } = storageData;
        const available_kb = total_kb - used_kb;
        
        // Update storage info
        document.getElementById('storageUsed').textContent = `${percent_used}%`;
        document.getElementById('storageAvailable').textContent = `${available_kb.toFixed(1)} KB`;
        document.getElementById('storageTotal').textContent = `${total_kb.toFixed(1)} KB`;
        
        // Update status based on usage
        let status = 'Good';
        let statusColor = '#28a745';
        
        if (percent_used > 85) {
            status = '⚠️ Almost Full';
            statusColor = '#dc3545';
        } else if (percent_used > 70) {
            status = '⚠️ High';
            statusColor = '#ffc107';
        }
        
        const statusElement = document.getElementById('storageStatus');
        statusElement.textContent = status;
        statusElement.style.color = statusColor;
        
        // Update progress bar
        const progressFill = document.getElementById('storageProgressFill');
        progressFill.style.width = `${percent_used}%`;
        
        // Change progress bar color based on usage
        let progressColor = '#28a745';
        if (percent_used > 85) {
            progressColor = '#dc3545';
        } else if (percent_used > 70) {
            progressColor = '#ffc107';
        } else if (percent_used > 50) {
            progressColor = '#17a2b8';
        }
        progressFill.style.background = progressColor;
        
        console.log(`Storage status updated: ${percent_used}% used (${used_kb.toFixed(1)}KB / ${total_kb.toFixed(1)}KB)`);
    }
}

// Initialize the controller when page loads
document.addEventListener('DOMContentLoaded', () => {
    window.flowController = new IntegratedFlowController();
});
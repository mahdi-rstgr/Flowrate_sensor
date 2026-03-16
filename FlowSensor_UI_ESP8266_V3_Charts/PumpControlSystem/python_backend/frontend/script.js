// Pump Controller JavaScript
class PumpController {
    constructor() {
        this.pumps = ['X', 'Y', 'Z'];
        this.pumpStates = {
            X: { running: false, rpm: 0, time: 0, continuous: false },
            Y: { running: false, rpm: 0, time: 0, continuous: false },
            Z: { running: false, rpm: 0, time: 0, continuous: false }
        };
        this.apiBaseUrl = 'http://localhost:5000/api';
        this.statusUpdateInterval = null;
        
        this.initializeEventListeners();
        this.updateDisplay();
        this.startStatusUpdates();
    }

    initializeEventListeners() {
        // Select All functionality
        document.getElementById('selectAll').addEventListener('change', (e) => {
            this.handleSelectAll(e.target.checked);
        });

        // Individual pump selection checkboxes
        document.querySelectorAll('.pump-checkbox').forEach(checkbox => {
            checkbox.addEventListener('change', () => {
                this.updateSelectAllState();
                this.updateSelectedCount();
            });
        });

        // Pump card selection checkboxes
        document.querySelectorAll('.pump-select').forEach(checkbox => {
            checkbox.addEventListener('change', (e) => {
                const pump = e.target.dataset.pump;
                const topCheckbox = document.getElementById(`select${pump}`);
                topCheckbox.checked = e.target.checked;
                this.updateSelectAllState();
                this.updateSelectedCount();
            });
        });

        // Start Selected Pumps button
        document.getElementById('startSelected').addEventListener('click', () => {
            this.startSelectedPumps();
        });

        // Emergency Stop button
        document.getElementById('emergencyStop').addEventListener('click', () => {
            this.emergencyStop();
        });

        // Individual pump controls
        this.pumps.forEach(pump => {
            // Start button
            document.querySelector(`.pump-start[data-pump="${pump}"]`).addEventListener('click', () => {
                this.startPump(pump);
            });

            // Stop button
            document.querySelector(`.pump-stop[data-pump="${pump}"]`).addEventListener('click', () => {
                this.stopPump(pump);
            });

            // RPM input
            document.querySelector(`.rpm-input[data-pump="${pump}"]`).addEventListener('change', (e) => {
                this.pumpStates[pump].rpm = parseFloat(e.target.value) || 0;
            });

            // Time input
            document.querySelector(`.time-input[data-pump="${pump}"]`).addEventListener('change', (e) => {
                this.pumpStates[pump].time = parseInt(e.target.value) || 0;
            });

            // Continuous operation checkbox
            document.querySelector(`.continuous-checkbox[data-pump="${pump}"]`).addEventListener('change', (e) => {
                this.pumpStates[pump].continuous = e.target.checked;
            });
        });
    }

    handleSelectAll(checked) {
        document.querySelectorAll('.pump-checkbox').forEach(checkbox => {
            checkbox.checked = checked;
        });
        document.querySelectorAll('.pump-select').forEach(checkbox => {
            checkbox.checked = checked;
        });
        this.updateSelectedCount();
    }

    updateSelectAllState() {
        const allCheckboxes = document.querySelectorAll('.pump-checkbox');
        const checkedCheckboxes = document.querySelectorAll('.pump-checkbox:checked');
        const selectAllCheckbox = document.getElementById('selectAll');
        
        selectAllCheckbox.checked = allCheckboxes.length === checkedCheckboxes.length;
        selectAllCheckbox.indeterminate = checkedCheckboxes.length > 0 && checkedCheckboxes.length < allCheckboxes.length;
    }

    updateSelectedCount() {
        const selectedCount = document.querySelectorAll('.pump-checkbox:checked').length;
        document.getElementById('selectedPumps').textContent = selectedCount;
    }

    async startSelectedPumps() {
        const selectedPumps = Array.from(document.querySelectorAll('.pump-checkbox:checked'))
            .map(checkbox => checkbox.dataset.pump);
        
        if (selectedPumps.length === 0) {
            this.showNotification('No pumps selected', 'warning');
            return;
        }

        for (const pump of selectedPumps) {
            await this.startPump(pump);
        }
    }

    async startPump(pump) {
        const state = this.pumpStates[pump];
        const rpm = parseFloat(document.querySelector(`.rpm-input[data-pump="${pump}"]`).value) || 0;
        const time = parseInt(document.querySelector(`.time-input[data-pump="${pump}"]`).value) || 0;
        const continuous = document.querySelector(`.continuous-checkbox[data-pump="${pump}"]`).checked;

        if (rpm === 0 || isNaN (rpm)) {
            this.showNotification(`${pump} Pump: RPM cannot be 0`, 'error');
            return;
        }

        try {
            const response = await fetch(`${this.apiBaseUrl}/start_pump`, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({
                    pump: pump,
                    rpm: rpm,
                    time: time,
                    continuous: continuous ? 1 : 0
                })
            });

            const result = await response.json();
            
            if (result.success) {
                this.pumpStates[pump].running = true;
                this.pumpStates[pump].rpm = rpm;
                this.pumpStates[pump].time = time;
                this.pumpStates[pump].continuous = continuous;
                this.updatePumpDisplay(pump);
                this.showNotification(`${pump} Pump started successfully`, 'success');
            } else {
                this.showNotification(`Failed to start ${pump} Pump: ${result.message}`, 'error');
            }
        } catch (error) {
            console.error('Error starting pump:', error);
            this.showNotification(`Error starting ${pump} Pump: ${error.message}`, 'error');
        }
    }

    async stopPump(pump) {
        try {
            const response = await fetch(`${this.apiBaseUrl}/stop_pump`, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({
                    pump: pump
                })
            });

            const result = await response.json();
            
            if (result.success) {
                this.pumpStates[pump].running = false;
                this.updatePumpDisplay(pump);
                this.showNotification(`${pump} Pump stopped successfully`, 'success');
            } else {
                this.showNotification(`Failed to stop ${pump} Pump: ${result.message}`, 'error');
            }
        } catch (error) {
            console.error('Error stopping pump:', error);
            this.showNotification(`Error stopping ${pump} Pump: ${error.message}`, 'error');
        }
    }

    async emergencyStop() {
        try {
            const response = await fetch(`${this.apiBaseUrl}/emergency_stop`, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                }
            });

            const result = await response.json();
            
            if (result.success) {
                this.pumps.forEach(pump => {
                    this.pumpStates[pump].running = false;
                    this.updatePumpDisplay(pump);
                });
                this.showNotification('Emergency stop activated - All pumps stopped', 'warning');
            } else {
                this.showNotification(`Emergency stop failed: ${result.message}`, 'error');
            }
        } catch (error) {
            console.error('Error during emergency stop:', error);
            this.showNotification(`Emergency stop error: ${error.message}`, 'error');
        }
    }

    async getStatus() {
        try {
            const response = await fetch(`${this.apiBaseUrl}/status`);
            const result = await response.json();
            
            if (result.success) {
                // Update pump states based on Arduino response
                this.pumps.forEach(pump => {
                    if (result.status[pump] !== undefined) {
                        this.pumpStates[pump].running = result.status[pump] === 1;
                        this.updatePumpDisplay(pump);
                    }
                });
                
                // Update system status
                document.getElementById('systemStatus').textContent = 'Ready';
                document.getElementById('systemStatus').className = 'status-value ready';
            } else {
                document.getElementById('systemStatus').textContent = 'Error';
                document.getElementById('systemStatus').className = 'status-value error';
            }
        } catch (error) {
            console.error('Error getting status:', error);
            document.getElementById('systemStatus').textContent = 'Disconnected';
            document.getElementById('systemStatus').className = 'status-value error';
        }
    }

    updatePumpDisplay(pump) {
        const statusIndicator = document.querySelector(`.status-indicator[data-pump="${pump}"]`);
        const statusText = document.querySelector(`.status-text[data-pump="${pump}"]`);
        const startButton = document.querySelector(`.pump-start[data-pump="${pump}"]`);
        const stopButton = document.querySelector(`.pump-stop[data-pump="${pump}"]`);

        if (this.pumpStates[pump].running) {
            statusIndicator.className = 'status-indicator running';
            statusText.textContent = 'Status: Running';
            startButton.disabled = true;
            stopButton.disabled = false;
        } else {
            statusIndicator.className = 'status-indicator stopped';
            statusText.textContent = 'Status: Stopped';
            startButton.disabled = false;
            stopButton.disabled = true;
        }
    }

    updateDisplay() {
        // Update active pumps count
        const activePumps = this.pumps.filter(pump => this.pumpStates[pump].running).length;
        document.getElementById('activePumps').textContent = activePumps;

        // Update selected pumps count
        this.updateSelectedCount();

        // Update last update time
        const now = new Date();
        document.getElementById('lastUpdate').textContent = now.toLocaleTimeString();

        // Update individual pump displays
        this.pumps.forEach(pump => {
            this.updatePumpDisplay(pump);
        });
    }

    startStatusUpdates() {
        // Update status every 2 seconds
        this.statusUpdateInterval = setInterval(() => {
            this.getStatus();
            this.updateDisplay();
        }, 2000);
    }

    stopStatusUpdates() {
        if (this.statusUpdateInterval) {
            clearInterval(this.statusUpdateInterval);
            this.statusUpdateInterval = null;
        }
    }

    showNotification(message, type = 'info') {
        // Create notification element
        const notification = document.createElement('div');
        notification.className = `notification notification-${type}`;
        notification.textContent = message;
        
        // Style the notification
        Object.assign(notification.style, {
            position: 'fixed',
            top: '20px',
            right: '20px',
            padding: '15px 20px',
            borderRadius: '8px',
            color: 'white',
            fontWeight: 'bold',
            zIndex: '1000',
            maxWidth: '300px',
            boxShadow: '0 4px 6px rgba(0, 0, 0, 0.1)'
        });

        // Set background color based on type
        switch (type) {
            case 'success':
                notification.style.backgroundColor = '#48bb78';
                break;
            case 'error':
                notification.style.backgroundColor = '#f56565';
                break;
            case 'warning':
                notification.style.backgroundColor = '#ed8936';
                break;
            default:
                notification.style.backgroundColor = '#4299e1';
        }

        // Add to page
        document.body.appendChild(notification);

        // Remove after 3 seconds
        setTimeout(() => {
            if (notification.parentNode) {
                notification.parentNode.removeChild(notification);
            }
        }, 3000);
    }
}

// Initialize the pump controller when the page loads
document.addEventListener('DOMContentLoaded', () => {
    window.pumpController = new PumpController();
});

// Handle page unload
window.addEventListener('beforeunload', () => {
    if (window.pumpController) {
        window.pumpController.stopStatusUpdates();
    }
});


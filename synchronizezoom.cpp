void MainWindow::synchronizeZoom(int zoom)
{
    // Prevent infinite recursion by temporarily blocking signals
    bool stagedBlocked = stagedContentEditor->blockSignals(true);
    bool currentBlocked = currentContentEditor->blockSignals(true);
    
    // Set the same zoom level for both editors
    if (sender() == stagedContentEditor) {
        currentContentEditor->setZoom(zoom);
    } else if (sender() == currentContentEditor) {
        stagedContentEditor->setZoom(zoom);
    }
    
    // Restore signals
    stagedContentEditor->blockSignals(stagedBlocked);
    currentContentEditor->blockSignals(currentBlocked);
}